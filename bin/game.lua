
-- Check if the user created a custom game we would want to run preferrably
if os.exists("../game.lua") then
	os.chdir("../")
	dofile("templates/init.lua")
	dofile("game.lua")
	return
end

-- We simply want to run the sample browser otherwise
useffmpegaudio = false -- don't use FFmpeg for the examples or the sample browser!
os.chdir("samplebrowser")
dofile("browser.lua")

