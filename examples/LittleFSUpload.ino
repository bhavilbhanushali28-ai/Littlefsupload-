#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <LittleFS.h>

// ===== WiFi Credentials =====
const char* ssid     = "realme5g";
const char* password = "bhavil23!";

// ===== Web Server on port 80 =====
ESP8266WebServer server(80);

// ===== HTML Upload Page =====
const char* uploadPage = R"rawhtml(
<!DOCTYPE html>
<html>
<head>
  <title>LittleFS Upload</title>
  <style>
    body { font-family: Arial, sans-serif; max-width: 500px; margin: 60px auto; text-align: center; }
    h2   { color: #333; }
    input[type=file] { margin: 20px 0; }
    input[type=submit] {
      background: #4CAF50; color: white;
      padding: 10px 30px; border: none;
      border-radius: 5px; cursor: pointer; font-size: 16px;
    }
    input[type=submit]:hover { background: #45a049; }
    #msg { margin-top: 20px; font-weight: bold; }
  </style>
</head>
<body>
  <h2>Upload File to LittleFS</h2>
  <form method="POST" action="/upload" enctype="multipart/form-data">
    <input type="file" name="file"><br>
    <input type="submit" value="Upload">
  </form>
  <div id="msg"></div>
  <br>
  <a href="/list">View uploaded files</a>
</body>
</html>
)rawhtml";

// ===== Handler: Root page =====
void handleRoot() {
  server.send(200, "text/html", uploadPage);
}

// ===== Handler: File Upload =====
void handleFileUpload() {
  HTTPUpload& upload = server.upload();

  if (upload.status == UPLOAD_FILE_START) {
    String filename = "/" + upload.filename;
    Serial.println("Uploading: " + filename);
    File f = LittleFS.open(filename, "w");
    if (!f) {
      Serial.println("Failed to open file for writing");
    }
    f.close();

  } else if (upload.status == UPLOAD_FILE_WRITE) {
    String filename = "/" + upload.filename;
    File f = LittleFS.open(filename, "a");
    if (f) {
      f.write(upload.buf, upload.currentSize);
      f.close();
    }

  } else if (upload.status == UPLOAD_FILE_END) {
    Serial.println("Upload complete: " + String(upload.totalSize) + " bytes");
  }
}

void handleUploadFinish() {
  server.send(200, "text/html",
    "<h2 style='font-family:Arial;text-align:center;margin-top:60px;color:green'>"
    "&#10003; Upload Successful!</h2>"
    "<p style='text-align:center;font-family:Arial'>"
    "<a href='/'>Upload another</a> | <a href='/list'>View files</a></p>"
  );
}

// ===== Handler: List files =====
void handleListFiles() {
  String html = "<html><body style='font-family:Arial;max-width:500px;margin:40px auto'>";
  html += "<h2>Files on LittleFS</h2><ul>";

  Dir dir = LittleFS.openDir("/");
  while (dir.next()) {
    html += "<li>" + dir.fileName() + " (" + String(dir.fileSize()) + " bytes)</li>";
  }

  html += "</ul><a href='/'>Back</a></body></html>";
  server.send(200, "text/html", html);
}

// ===== Handler: Download a file =====
void handleFileDownload() {
  String path = server.arg("name");
  if (!path.startsWith("/")) path = "/" + path;

  if (!LittleFS.exists(path)) {
    server.send(404, "text/plain", "File not found");
    return;
  }

  File f = LittleFS.open(path, "r");
  server.streamFile(f, "application/octet-stream");
  f.close();
}

// ===== Setup =====
void setup() {
  Serial.begin(115200);
  delay(500);
  Serial.println("\n\n--- LittleFS Web Uploader ---");

  // Mount LittleFS
  if (!LittleFS.begin()) {
    Serial.println("ERROR: LittleFS mount failed! Formatting...");
    LittleFS.format();
    LittleFS.begin();
  }
  Serial.println("LittleFS mounted OK");

  // Connect to WiFi
  Serial.print("Connecting to WiFi: ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);

  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 30) {
    delay(500);
    Serial.print(".");
    attempts++;
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\nConnected!");
    Serial.print("Open browser at: http://");
    Serial.println(WiFi.localIP());
  } else {
    Serial.println("\nWiFi connection FAILED. Check credentials.");
  }

  // Register routes
  server.on("/",        HTTP_GET,  handleRoot);
  server.on("/list",    HTTP_GET,  handleListFiles);
  server.on("/download",HTTP_GET,  handleFileDownload);
  server.on("/upload",  HTTP_POST, handleUploadFinish, handleFileUpload);

  server.begin();
  Serial.println("HTTP server started");
}

// ===== Loop =====
void loop() {
  server.handleClient();
}
