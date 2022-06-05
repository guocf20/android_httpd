local socket = require("socket")
local host, port = "127.0.0.1", 6000
local tcp = assert(socket.tcp())

tcp:connect(host, port);
tcp:send("hello world\n");

while true do
    local s, status, partial = tcp:receive("*l")
    print(s or partial)
    if status == "closed" then
      break
    end
end

tcp:close()
