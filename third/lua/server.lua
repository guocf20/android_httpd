loadfile("socket.lua")()
local socket = require('socket')
local server = socket.bind('*',12345)
local client = server:accept()
