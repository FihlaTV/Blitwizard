
if not reload then
    reload = function()
        dofile(os.gameluapath())
    end
    if type(os) == "table" then
        os.reload = reload
    end
end


