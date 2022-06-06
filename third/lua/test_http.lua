local http = require("http")
local ltn12 = require("ltn12")
r, e = http.request("http://heike01.com/")
print(r)
