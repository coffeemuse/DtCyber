const fs = require("fs");
const Hasp = require("./hasp");
const process = require("process");
const readline = require("readline");

const configFile = (process.argv.length > 2) ? process.argv[2] : "config.json";
const config = JSON.parse(fs.readFileSync("config.json"));
const defaults = {
  host: "localhost",
  port: 2553,
  spoolDir: "./spool"
};
for (const key of Object.keys(defaults)) {
  if (typeof config[key] === "undefined") config[key] = defaults[key];
}

let streams = {};

function cli() {

  const rl = readline.createInterface({
    input: process.stdin,
    output: process.stdout,
    prompt: "\nOperator> "
  });

  rl.on("line", line => {
    const argv = line.trim().split(" ");
    if (argv.length > 0) {
      switch (argv[0]) {
      case "lc":
      case "load_cards":
        if (argv.length > 1) {
          loadCards(1, argv[1]);
        }
        else {
          process.stdout.write("Please provide a file name\n");
        }
        break;
      case "exit":
      case "quit":
        process.exit(0);
        break;
      case "":
        break;
      default:
        process.stdout.write("Unrecognized command\n");
        break;
      }
    }
    rl.prompt();
  });

  rl.on("close", () => {
    process.stdout.write("\nRJE CLI exiting\n");
    process.exit(0);
  });

  rl.prompt();
}

function loadCards(streamId, path) {
  try {
    const key = `CR${streamId}`;
    if (typeof streams[key] !== "undefined" && streams[key].isBusy) {
      process.stdout.write(`${key} is busy\n`);
      return;
    }
    fs.accessSync(path, fs.constants.R_OK);
    streams[key] = {};
    streams[key].path = path;
    streams[key].isBusy = true;
    process.stdout.write(`Loading ${path} on ${key} ...`);
    hasp.requestToSend(streamId);
  }
  catch (err) {
    process.stdout.write(`Failed to open ${path}: ${err}\n`);
  }
}

process.stdout.write("\nRJE CLI starting ...");

const hasp = new Hasp(config);

hasp.on("data", (recordType, streamId, data) => {
  switch (recordType) {
  case Hasp.RecordType_OpMsg:
    process.stdout.write(`\n${data}`);
    break;
  case Hasp.RecordType_PrintRecord:
  case Hasp.RecordType_PunchRecord:
    const key = `${recordType === Hasp.RecordType_PrintRecord ? "LP" : "CP"}${streamId}`;
    if (typeof streams[key] === "undefined"
        || typeof streams[key].path === "undefined"
        || typeof streams[key].stream === "undefined") {
      streams[key] = {};
      const date = new Date();
      let path = `${config.spoolDir}/${key}_${date.getFullYear()}`;
      if (date.getMonth() < 9) path += "0";
      path += date.getMonth() + 1;
      if (date.getDate() < 10) path += "0";
      path += date.getDate();
      if (date.getHours() < 10) path += "0";
      path += date.getHours();
      if (date.getMinutes() < 10) path += "0";
      path += date.getMinutes();
      if (date.getSeconds() < 10) path += "0";
      path += date.getSeconds();
      streams[key].path = path;
      try {
        streams[key].stream = fs.createWriteStream(path, {encoding:"utf8",mode:"660"});
        process.stdout.write(`\nCreated ${path}`);
      }
      catch (err) {
        process.stdout.write(`\nFailed to create ${path}: ${err}`);
        return;
      }
    }
    if (data !== null) {
      streams[key].stream.write(`${data}\n`);
    }
    else {
      streams[key].stream.close();
      process.stdout.write(`\nClosed ${streams[key].path}`);
      delete streams[key];
    }
    break;
  default:
    // ignore other record types
    break;
  }
});

hasp.on("end", streamId => {
  const key = `CR${streamId}`;
  const path = streams[key].path;
  streams[key].isBusy = false;
  process.stdout.write(`\nDone    ${path} on ${key} ...`);
});

hasp.on("signon", () => {
  process.stdout.write(`\nConnected to HASP host at ${config.host}:${config.port}`);
  cli();
});

hasp.on("pti", (recordType, streamId) => {
  const key = `CR${streamId}`;
  const path = streams[key].path;
  const stream = fs.createReadStream(path);
  streams[key].stream = stream;
  process.stdout.write(`\nReading ${path} on ${key} ...`);
  hasp.send(streamId, stream);
});

hasp.start();
