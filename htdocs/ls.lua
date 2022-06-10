#!/usr/bin/lua
local lfs = require"lfs"
basepath="htdocs"
function attrdir (path)
    for file in lfs.dir(path) do
        if file ~= "." and file ~= ".." then
            local f = path..'/'..file
            --print ("\t "..f)
            local attr = lfs.attributes (f)
            assert (type(attr) == "table")
            if attr.mode == "directory" then
                attrdir (f)
            else
		    entry =string.format("<tr><td><a href=\"%s\">%s</a></td><td align=\"right\">%d  </td></tr>", string.sub(f, 7),string.sub(f, 7), attr.size)
		    print(entry)
		  --  print(string.sub(f, 7), attr.size, attr.permissions)
            end
        end
    end
end

print("<html><head><title>Index of /lua_src</title></head><body><h1>Index of /lua_src</h1>")
print(" <table>")
attrdir (arg[1])
print(" </table>")
print("</body></html>")
