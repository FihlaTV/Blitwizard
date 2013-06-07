
-- Avoid double initialisation
if blitwizard.templatesinitialised == true then
	return
end
blitwizard.templatesinitialised = true

-- Load all the templates
if os.sysname() ~= "Android" then
	-- Crawl the templates/ folder for templates
	local templates = os.templatedir()
    if templates == nil then
        os.forcetemplatedir("templates/")
        templates = "templates/"
    end
    print("template init...")
	for index,file in ipairs(os.ls(templates)) do
        print("file: " .. file)
		if os.isdir(templates .. "/" .. file) then
			local filepath = templates .. "/" .. file .. "/" .. file .. ".lua"
			if os.exists(filepath) then
				dofile(filepath)
			end
		end
	end
    print("template init end")
else
	-- For android, we get a pre-generated Lua file
    -- (provided by the android build script in scripts/ folder
    -- which assembled the .apk package)
	dofile("templates/filelist.lua")
end

