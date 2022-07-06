# stdio-tcp

Small tool to a pipe STDIO to a TCP and try to recover as much as possible from
IO errors.

## Typical usage

SSH server forbids tunnels, but you need one and have a shell:

### Server side
```bash
curl -LO 'https://github.com/jpillora/chisel/releases/download/v1.7.7/chisel_1.7.7_linux_amd64.gz'
gzip -dc 'chisel_1.7.7_linux_amd64.gz' > chisel
chmod +x chisel
./chisel server --socks5 --reverse --host 127.0.0.1 --port 1337
```

### Client side
```bash
rm -f /tmp/f
mkfifo /tmp/f
scp stdio-tcp $SERVER:/tmp
cat /tmp/f | \
  socat TCP-LISTEN:1337,reuseaddr=1 STDIO | \
  ssh $SERVER '/tmp/stdio-tcp 127.0.0.1 1337' \
  >> /tmp/f
```

Now, enjoy your chisel !

## How to compile
This tool is linked against [musl](https://www.musl-libc.org/) to avoid errors
like:
```
version `GLIBC_2.29’ not found (required by ./stdio-tcp)
```
Once installed, just run `make`
