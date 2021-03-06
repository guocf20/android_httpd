#!/usr/bin/lua
local lfs = require"lfs"
rootdir="htdocs"
basepath="files"
function basename(str)
	local name = string.gsub(str, "(.*/)(.*)", "%2")
	return name
end

function dirname(str)
	if str:match(".-/.-") then
		local name = string.gsub(str, "(.*/)(.*)", "%1")
		return name
	else
		return ''
	end
end

function attrdir (path)
    for file in lfs.dir(path) do
        if file ~= "." and file ~= ".." then
            local f = path..'/'..file
            --print ("\t "..f)
            local attr = lfs.attributes (f)
            assert (type(attr) == "table")
            if attr.mode == "directory" then

		    	entry =string.format("<tr><td valign=\"top\"><img src=\"/icons/folder.gif\" alt=\"[   ]\"></td><td><a href=\"%s\">%s</a></td><td align=\"right\">%d  </td></tr>", string.sub(f, 7),file, attr.size)
		    print(entry)
            else
		    if string.find(file, ".zip") then    
		    	entry =string.format("<tr><td valign=\"top\"><img src=\"/icons/compressed.gif\" alt=\"[   ]\"></td><td><a href=\"%s\">%s</a></td><td align=\"right\">%d  </td></tr>", string.sub(f, 7),file, attr.size)

		    elseif string.find(file, ".jpg") then
		    	entry =string.format("<tr><td valign=\"top\"><img src=\"/icons/image2.gif\" alt=\"[   ]\"></td><td><a href=\"%s\">%s</a></td><td align=\"right\">%d  </td></tr>", string.sub(f, 7),file, attr.size)
		    else 
		    entry =string.format("<tr><td valign=\"top\"><img src=\"/icons/unknown.gif\" alt=\"[   ]\"></td><td><a href=\"%s\">%s</a></td><td align=\"right\">%d  </td></tr>", string.sub(f, 7),file, attr.size)
		    end 

		    print(entry)
		  --  print(string.sub(f, 7), attr.size, attr.permissions)
            end
        end
    end
end

--header=string.format("<html><head><title>Index of %s</title></head><body><h1>Index of %s</h1>", string.sub(arg[1], 7), string.sub(arg[1], 7))
header=string.format("<html><head><title>Index of %s</title><meta http-equiv=\"Content-Type\" content=\"text/html; charset=utf-8\"></head><body><h1>Index of %s</h1>", string.sub(arg[1], 7), string.sub(arg[1], 7))
print(header)
print(" <table>")
tmp  = dirname(arg[1])
entry =string.format("<tr><td valign=\"top\"><img src=\"/icons/back.gif\" alt=\"[   ]\"></td><td><a href=\"%s\">%s</a></td></tr>", string.sub(tmp, 7), "Parent Directory")
print(entry)
attrdir (arg[1])
print(" </table>")
print("</body></html>")
