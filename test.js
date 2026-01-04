const RPC = require("discord-rpc");

const clientId = "585817538412150794";
const RECONNECT_DELAY = 30_000;

let rpc;
let reconnectTimeout = null;
let isConnecting = false;

RPC.register(clientId);

function startRPC() {
    if (isConnecting) return;
    isConnecting = true;

    rpc = new RPC.Client({ transport: "ipc" });

    rpc.on("ready", () => {
        isConnecting = false;
        console.log("RPC connected");

        rpc.setActivity({
            startTimestamp: Date.now(),
            buttons: [
                { label: "GitHub", url: "https://github.com" }
            ]
        });
    });

    rpc.on("disconnected", () => {
        console.log("RPC disconnected");
        scheduleReconnect();
    });

    rpc.on("error", (err) => {
        console.log("RPC error:", err.message);
        scheduleReconnect();
    });

    rpc.login({ clientId }).catch(() => {
        scheduleReconnect();
    });
}

function scheduleReconnect() {
    if (reconnectTimeout) return;

    isConnecting = false;

    reconnectTimeout = setTimeout(() => {
        reconnectTimeout = null;
        console.log("Reconnecting RPC...");
        startRPC();
    }, RECONNECT_DELAY);
}

// 🔒 Chặn crash hoàn toàn
process.on("uncaughtException", () => {});
process.on("unhandledRejection", () => {});

startRPC();
