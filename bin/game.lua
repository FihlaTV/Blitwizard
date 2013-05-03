
-- open graphics output:
blitwizard.graphics.setMode(800, 600, "TEST", false)

-- zoom more into things:
--blitwizard.graphics.getCameras()[1]:set2dZoomFactor(2)

-- blue orb spawn function:
function spawnOrb()
    -- spawn blue orb at random position:
    local obj = blitwizard.object:new(false, "orb.png")
    obj:setPosition(math.random()*6-3, math.random()*6-3)

    -- set random scale:
    local v = math.random()
    obj:setScale(v, v)

    -- make it fall down:
    obj.speed = {x = 0.2 * math.random() - 0.1, y = -0.05 * math.random()}
    function obj:doAlways()
        -- accelerate:
        obj.speed.y = obj.speed.y + 0.0005

        -- set new position:
        local x,y = self:getPosition()
        self:setPosition(x + obj.speed.x, y + obj.speed.y)

        -- if too far below, delete:
        if y > 3 then
            self:destroy()

            -- spawn new:
            --spawnOrb()
        end
    end
end

function massSpawn()
    -- spawn 20 orbs:
    local i = 0
    while i < 1 do
        spawnOrb()
        i = i + 1
    end
end

massSpawn()

