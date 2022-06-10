str = string.format("ls -lhtr %s", arg[1])
--for dir in io.popen([[ls -lhtr . ]]):lines() 
for dir in io.popen(str):lines() 
	do 
		print(dir) 
	end
