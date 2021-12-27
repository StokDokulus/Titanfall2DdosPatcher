# Titanfall2 DeltaBuf patch
This patch for Titanfall 2 helps prevent disconnects while the servers are being attacked by a DoS attack.
## Disclaimer
This is experimental software which is provided "as is", without warranty of any kind.
Use it at your own risk.

## How to use this patch
- Using the patcher: Just download (or build) the patcher and run it.
- Manually: The file `tif2_larger_deltabuf_v1.1337` can be used with [x64dbg](https://x64dbg.com/) to apply the patch manually:
  - Start Titanfall 2 and attach x64dbg to the Titanfall2 process
  - Load the patch (File > Patch File > Import)
  - Write the patched `engine.dll` using `Patch File`.

## How it works
The DoS attacks on the Titanfall2 servers currently (Dec 2021) cause large lagspikes every few minutes.
During such a lagspike, the server still sends updates to the clients but seems to be incapable of processing any data from the clients.
The updates the client receives from the server pile up in the client's *Delta Buffer* for the duration of the lagspike.
Presumably, this happens because some of these updates are lost and the server can't respond to retransmission requests. Since the game (which is base d on Source Engine) uses delta compression, the client has to stash the received data until it can fetch the missing updates from the server.

Since the Delta Buffer has a fixed size of 16MiB, it will overflow after a few seconds which causes the client to disconnect.
This patch increases the Delta Buffer size to 64MiB which seems to be enough to prevent overflows.

## Building the Patcher
The patcher can be built from source using [Qt Creator](https://www.qt.io/download-open-source).
