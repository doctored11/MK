const http = require('http');
const fs = require('fs');
const { SerialPort } = require('serialport');

const port = new SerialPort({ path: 'COM6', baudRate: 115200 });

let debounceTimeout = null;
let lastMessage = '';

function sendToSerialWithDebounce(message, delay = 500) {
  lastMessage = message;
  if (debounceTimeout) clearTimeout(debounceTimeout);

  debounceTimeout = setTimeout(() => {
    console.log('->', lastMessage);
    port.write(lastMessage);
  }, delay);
}

const server = http.createServer((req, res) => {
  if (req.url === '/') {
    fs.readFile('index.html', (err, data) => {
      
      res.writeHead(200, { 'Content-Type': 'text/html' });
      res.end(data);
    });
  } else if (req.url.startsWith('/set')) {
    const url = new URL(req.url, `http://${req.headers.host}`);
    const r = url.searchParams.get('r');
    const g = url.searchParams.get('g');
    const b = url.searchParams.get('b');

    const message = `${r}-${b}-${g}\n`;
    sendToSerialWithDebounce(message);

    res.writeHead(200);
    res.end('OK');
  } else {
    res.writeHead(404);
    res.end('Not found');
  }
});

server.listen(3000, () => {
  console.log('Сервер запущен: http://localhost:3000');
});
