"""b0r4-watch server — entry point FastAPI."""

import json
import time
from collections import deque
from contextlib import asynccontextmanager

import uvicorn
from fastapi import FastAPI, WebSocket, WebSocketDisconnect

from src.asr import get_asr_provider
from src.audio import AudioBuffer
from src.config import settings
from src.llm import get_llm_provider
from src.music import next_track, pause, play
from src.tts import get_tts_provider


@asynccontextmanager
async def lifespan(app: FastAPI):  # noqa: ARG001
    app.state.asr = get_asr_provider()
    app.state.llm = get_llm_provider()
    app.state.tts = get_tts_provider()
    yield


app = FastAPI(title="b0r4-watch server", lifespan=lifespan)

# --- In-memory state ---

_music_state: dict = {
    "title": "",
    "artist": "",
    "isPlaying": False,
    "duration": 0,
    "position": 0,
}

_notifications: deque = deque(maxlen=50)

_gps_location: dict = {"latitude": 0.0, "longitude": 0.0, "source": "none"}


@app.get("/health")
async def health():
    """Health check usado pelo relógio para detectar o servidor."""
    return {
        "status": "ok",
        "project": "b0r4-watch",
        "asr_provider": settings.asr_provider,
        "llm_provider": settings.llm_provider,
        "tts_provider": settings.tts_provider,
    }


@app.websocket("/ws/audio")
async def audio_stream(websocket: WebSocket):
    """Stream de áudio do relógio para ASR -> LLM -> TTS.

    Protocolo:
    - JSON {"type": "start"}  -> inicia gravação
    - binary frames           -> chunks de áudio PCM 16kHz mono 16-bit
    - JSON {"type": "end"}    -> finaliza e processa
    - Server responde com JSON (text, tool_call, audio) e bytes de áudio.
    """
    await websocket.accept()
    buffer = AudioBuffer()

    try:
        while True:
            message = await websocket.receive()

            if "text" in message:
                try:
                    data = json.loads(message["text"])
                except json.JSONDecodeError:
                    await websocket.send_json(
                        {"type": "error", "message": "Invalid JSON"}
                    )
                    continue

                msg_type = data.get("type")

                if msg_type == "start":
                    buffer.reset()
                    await websocket.send_json(
                        {"type": "status", "message": "listening"}
                    )

                elif msg_type == "end":
                    await websocket.send_json(
                        {"type": "status", "message": "processing"}
                    )
                    audio_bytes = buffer.to_wav_bytes()

                    transcription = await websocket.app.state.asr.transcribe(
                        audio_bytes, language=settings.assistant_language
                    )
                    await websocket.send_json(
                        {"type": "text", "role": "user", "content": transcription}
                    )

                    result = await websocket.app.state.llm.chat(
                        transcription, language=settings.assistant_language
                    )
                    assistant_text = result["text"]

                    await websocket.send_json(
                        {
                            "type": "text",
                            "role": "assistant",
                            "content": assistant_text,
                        }
                    )

                    for tc in result.get("tool_calls", []):
                        await websocket.send_json({"type": "tool_call", "data": tc})

                    if assistant_text:
                        audio_response = await websocket.app.state.tts.synthesize(
                            assistant_text, language=settings.assistant_language
                        )
                        audio_format = "wav" if settings.tts_provider == "mock" else "mp3"
                        await websocket.send_json(
                            {"type": "audio", "format": audio_format}
                        )
                        await websocket.send_bytes(audio_response)

                    buffer.reset()

                else:
                    await websocket.send_json(
                        {"type": "error", "message": f"Unknown type: {msg_type}"}
                    )

            elif "bytes" in message:
                buffer.append(message["bytes"])

    except WebSocketDisconnect:
        pass
    except Exception as exc:
        try:
            await websocket.send_json({"type": "error", "message": str(exc)})
        except Exception:
            pass


def _music_track_info() -> dict:
    """Return current music state as a dict."""
    return {**_music_state}


# ── Task 12: /ws/music WebSocket ─────────────────────────────────────────────


@app.websocket("/ws/music")
async def music_stream(websocket: WebSocket):
    """Controle de música via WebSocket.

    Protocolo:
    - JSON {"type": "command", "action": "play"|"pause"|"next"|"prev"}
    - Server responde com JSON com info da faixa atual.
    - Server pode enviar atualizações de faixa (push).
    """
    await websocket.accept()

    try:
        # Envia estado atual ao conectar
        await websocket.send_json(_music_track_info())

        while True:
            message = await websocket.receive()

            if "text" not in message:
                continue

            try:
                data = json.loads(message["text"])
            except json.JSONDecodeError:
                await websocket.send_json({"error": "Invalid JSON"})
                continue

            if data.get("type") != "command":
                await websocket.send_json({"error": "Expected type 'command'"})
                continue

            action = data.get("action")
            now = time.time()

            if action == "play":
                result = await play(data.get("query"))
                _music_state["isPlaying"] = True
                _music_state["position"] = 0
                if not _music_state["title"]:
                    _music_state["title"] = result
                    _music_state["artist"] = ""
                _music_state["duration"] = 210

            elif action == "pause":
                await pause()
                _music_state["isPlaying"] = False

            elif action == "next":
                await next_track()
                _music_state["isPlaying"] = True
                _music_state["position"] = 0
                _music_state["title"] = "Próxima faixa"
                _music_state["artist"] = ""
                _music_state["duration"] = 240

            elif action == "prev":
                _music_state["isPlaying"] = True
                _music_state["position"] = 0
                _music_state["title"] = "Faixa anterior"
                _music_state["artist"] = ""
                _music_state["duration"] = 200

            else:
                await websocket.send_json({"error": f"Unknown action: {action}"})
                continue

            await websocket.send_json(_music_track_info())

    except WebSocketDisconnect:
        pass
    except Exception as exc:
        try:
            await websocket.send_json({"error": str(exc)})
        except Exception:
            pass


# ── Task 13: /api/notifications and /api/gps ─────────────────────────────────


@app.post("/api/notifications")
async def receive_notification(body: dict):
    """Recebe notificação do firmware para logging."""
    _notifications.append(
        {
            "app": body.get("app", ""),
            "title": body.get("title", ""),
            "message": body.get("message", ""),
            "timestamp": body.get("timestamp", time.time()),
            "received_at": time.time(),
        }
    )
    return {"status": "ok"}


@app.get("/api/gps")
async def get_gps():
    """Retorna última localização conhecida."""
    return {**_gps_location}


if __name__ == "__main__":
    uvicorn.run("main:app", host="0.0.0.0", port=8000, reload=True)
