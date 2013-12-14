--[[
blitwizard.net.http
Under the zlib license:

Copyright (c) 2012-2013 Jonas Thiem

This software is provided 'as-is', without any express or implied warranty. In no event will the authors be held liable for any damages arising from the use of this software.

Permission is granted to anyone to use this software for any purpose, including commercial applications, and to alter it and redistribute it freely, subject to the following restrictions:

1. The origin of this software must not be misrepresented; you must not claim that you wrote the original software. If you use this software in a product, an acknowledgment in the product documentation would be appreciated but is not required. 

2. Altered source versions must be plainly marked as such, and must not be misrepresented as being the original software.

3. This notice may not be removed or altered from any source distribution.

]]

--[[--
 This is the HTTP module which allows you to fetch web documents.
 You can use this to automatically download patches/updates, high score lists
 etc.
 @module blitwizard.net.http
]]

blitwizard.net.http = {}

--[[--
 This illustrates the response table you get from the @{get|http.get function's
 reponse_callback}.
 @table response
 @tfield number response_code the HTTP response code. 200 means success,
  anything else indicates an error. All errors are usually indicated by the server,
  search the internet for HTTP response codes if you need more detailed information
  on how to interpret them. The exception is response code 999, which issued
  locally by blitwizard in case the server couldn't be contacted at all.
 @tfield content string the received content. In case of an error code, it usually
  contains the error message (or nothing).
 @tfield mime_type string the mime type which the server reports for the document,
  e.g. "text/plain" for plain text documents, "text/html" for HTML documents, etc.
 @tfield server_name string the server name (as reported by the server itself).
  May be nil on failure of if the server doesn't report a name.
 @tfield headers string contains a string with all the response HTTP headers
  sent by the server. This is for advanced HTTP tricks, and usually not needed.
]]

--[[--
 Use this function to get a web document.

 The function will issue a callback once the requested document is available.
 @function get
 @tparam string url the url of the web document, e.g. http://www.blitwizard.de/
 @tparam function response_callback the function which will be called once the
  document arrived (or when arrival failed). The first and only parameter is a
  @{response} table that contains the document (or the error on failure) and
  various additional information.
 @usage
   blitwizard.net.http.get("http://www.blitwizard.de/",
       function(response)
           if response.response_code == 200 then
               print("Document arrived! Here it is:\n" .. response.content)
           else
               print("Failed to obtain document! Error code: " ..
                   response.response_code)
           end
       end
   )
]]

blitwizard.net.http.get = function(url, callback, headers)
    --   *** Blitwizard HTTP interface ***
    --
    -- The blitwizard http interface consists of this function.
    -- Use it as follows:
    --     blitwizard.net.http.get("http://some/url/",
    --         function(response)
    --             --[[ do something here ]]
    --         end
    --     )
    -- The function you provide gets a response object which is a
    -- table with the following members:
    --    data.response_code - contains the HTTP response code, so
    --                          200 when everything went fine.
    --                         There is also 404, which indicates
    --                          page not found,
    --                          and 403 for access forbidden, and more.
    --                         Return code 999 means the connection failed
    --                          for some reason and is a LOCAL error code,
    --                          not something actually sent by the server.
    --    data.content       - the page content of the response,
    --                          containing HTML code, image data,
    --                          or whatever you requested. For return code
    --                          999, it contains the error message.
    --    data.mime_type     - contains the content's mime type, e.g.
    --                          "text/plain" for plain text, "text/html"
    --                          for HTML or "audio/ogg" for ogg audio.
    --    data.server_name   - contains a string containing the server's
    --                          name, or nil if not told by the server.
    --    data.headers       - contains a string with all the response
    --                          http headers sent by the server.
    -- IMPORTANT: Alternatively, the response object is simply nil
    -- when the request failed completely (network error or similar).
    --
    -- As an example, print the HTML code of a website like this:
    --     blitwizard.net.http.get("http://www.blitwizard.de/",
    --         function(r)
    --             print r.content
    --         end
    --     )
    --
    -- The headers parameter allows you to specify additional headers
    -- like another user agent, cache control or similar things.
    -- Specify them in a string containing multiple lines
    -- separated through \n, e.g. like this:
    --   "User-agent: blubb\nX-Hello: bla"

    if type(url) ~= "string" then
        error("bad argument #1 to `blitwizard.net.http.get` " ..
        "(string expected, got " .. type(url) .. ")")
    end

    -- first, check and remove http://
    if not string.startswith(url, "http://") then
        error("bad argument #1 to `blitwizard.net.http.get`: " ..
        "not a http link (please remember https is not supported)")
    end
    url = string.sub(url, #"http://"+1)

    -- extract server name:
    local server_name = string.split(url, ":", 1)
    server_name = string.split(server_name, "/", 1)
    if #server_name <= 0 then
        error("bad argument #1 to `blitwizard.net.http.get`: " ..
        "url has empty target server name")
    end

    local portspecified = false
    if #server_name + 1 == string.find(url, ":", 1, true) then
        portspecified = true
    end
    url = string.sub(url, #server_name + 2)

    -- extract port:
    local port = "80"
    if #url > 0 and portspecified == true then
        port = string.split(url, "/", 1)
        url = string.sub(url, #port + 1)
    end

    -- validate port
    if tostring(tonumber(port)) ~= port then
        error("bad argument #1 to `blitwizard.net.http.get`: " ..
        "port not a number")
    end
    port = tonumber(port)
    if port < 1 or port > 65535 then
        error("bad argument #1 to `blitwizard.net.http.get`: " ..
        "port number exceeds valid port range")
    end

    -- obtain resource
    local resource = url
    if #resource <= 0 then
        resource = "/"
    end
    resource = string.gsub(resource, " ", "%20")
    if not string.startswith(resource, "/") then
        resource = "/" .. resource
    end

    if headers == nil then
        headers = ""
    end

    local final_headers = blitwizard.net.http._merge_headers(
        "GET " .. resource .. " HTTP/1.1\n" ..
        "Connection: Close\n" ..
        "Accept-Encoding: identity;q=1 *;q=0\n" ..
        "Host: " .. server_name .. "\n"
    ,
    headers
    )

    if not blitwizard.net.http._header_present(final_headers,
    "User-agent: ") then
        final_headers = final_headers .. "User-agent: " .. 
        blitwizard.net.http._default_user_agent .. "\n"
    end

    local streamdata = {}
    local stream = blitwizard.net.connection:new(
    {server=server_name, port=port},
    function(self)
        self:send(final_headers .. "\n")
    end,
    function(self, data)
        if streamdata["data"] == nil then
            streamdata["data"] = ""
        end
        streamdata["data"] = streamdata["data"] .. data
    end,
    function(self, errormsg)
        if streamdata["data"] ~= nil then
            --print("blib")
            if #streamdata["data"] > 0 then
                local data = {}
                data["headers"] = ""
                data["response_code"] = 200
                --parse response code:
                local datastr = streamdata["data"]
                if string.startswith(datastr, "HTTP/") then
                    local blub,response_code = string.split(datastr, " ", 2)
                    if tostring(tonumber(response_code)) == response_code then
                        if tonumber(response_code) ~= 200 then
                            -- error response
                            data["response_code"] = tonumber(response_code)
                        end
                    end
                end

                -- it worked apparently! get us the data:
                local header,body = string.split(datastr, "\n\n", 1)
                data.content = body
                data.headers = header
                local header2,body2 = string.split(datastr, "\r\n\r\n", 1)
                if #header2 < #data.headers then
                    data.headers = header2
                    data.content = body2
                end
                data.headers = string.gsub(data.headers, "\r\n", "\n") 

                -- Make sure we have proper content and header (not nil)
                if data.content == nil then
                    data.content = ""
                    if data.headers ~= nil then
                        if string.find(data.headers, "<html>") ~= nil then
                            -- nginx likes to sent without header sometimes
                            -- (for bad requests)
                            data.content = data.headers
                            data.headers = ""
                        end
                    end
                end
                if data.headers == nil then
                    data.headers = ""
                end

                -- separate headers
                local headerstable = {string.split(data.headers, "\n")}
                
                -- parse for various options
                local chunked = false
                local i = 1
                while i <= #headerstable do
                    headerstable[i] = string.gsub(headerstable[i], " :", ":")
                    headerstable[i] = string.gsub(headerstable[i], ": ", ":")
                    
                    -- chunked transfer encoding
                    if string.startswith(string.lower(headerstable[i]),
                    "transfer-encoding:") then
                        local v = ({string.split(string.lower(headerstable[i]),
                        ":", 1)})[2]
                        if v == "chunked" then
                            chunked = true
                        end
                    end
 
                    -- mime type
                    if string.startswith(string.lower(headerstable[i]),
                    "content-type:") then
                        data.mime_type = ({string.split(
                        string.lower(headerstable[i]), ":", 1)})[2]
                    end

                    -- server name
                    if string.startswith(string.lower(headerstable[i]),
                    "server:") then
                        data.server_name = ({string.split(
                        string.lower(headerstable[i]), ":", 1)})[2]
                    end

                    i = i + 1
                end

                -- parse chunks in data if chunked
                if chunked == true then
                    local dechunkeddata = ""
                    while 1 do
                        local newdata = ""

                        -- Trim leading chunk size line break if any
                        if string.startswith(data.content, "\r") then
                            data.content = string.sub(data.content, 2)
                        end
                        if string.startswith(data.content, "\n") then
                            data.content = string.sub(data.content, 2)
                        end

                        -- Invalid data if nothing is left:
                        if #data.content <= 0 then
                            data.response_code = 999
                            data.headers = ""
                            data.mime_type = "text/plain"
                            data.content = "Invalid chunked data"
                            break
                        end

                        local chunklengthstr,newdata = string.split(
                        data.content, "\r\n", 1)
                        data.content = newdata
                        local chunklength = tonumber("0x" .. chunklengthstr)
                        
                        -- Stop splitting if final chunk
                        if chunklength <= 0 then
                            data.content = dechunkeddata
                            break
                        end

                        -- Trim leading data line break:
                        if string.startswith(data.content, "\r") then
                            data.content = string.sub(data.content, 1)
                        end
                        if string.startswith(data.content, "\n") then
                            data.content = string.sub(data.content, 1)
                        end

                        -- Invalid if remaining data is too short:
                        if #data.content < chunklength then
                            data.response_code = 999
                            data.headers = ""
                            data.mime_type = "text/plain"
                            data.content = "Received data incomplete"
                            break
                        end

                        -- Cut off chunk
                        dechunkeddata = dechunkeddata ..
                        string.sub(data.content, 1, chunklength)
                        data.content = string.sub(data.content,
                        chunklength + 1)
                    end
                end              
  
                callback(data)
                return
            end
        end
        callback({response_code=999, headers="", mime_type="text/plain",
        content=errormsg})
    end
) 
end


blitwizard.net.http._default_user_agent = "blitwizard.net.http/1.0"
blitwizard.net.http._header_present = function(headerblock, header)
    while string.find(header, " :") ~= nil do
        string.gsub(header, " :", ":")
    end
    header = string.sub(header, 1, string.find(header, ":"))
    header = string.lower(header)
    headerblock = string.lower(headerblock)

    if string.startswith(headerblock, header) then
        return true
    end
    if string.find(headerblock, header, 1, true) ~= nil then
        return true
    end
    return false
end

blitwizard.net.http._merge_headers = function(headerblock1, headerblock2)
    -- Merge headers and remove duplicated entries
    local lines = {string.split(headerblock2, "\n")}
    for number,line in ipairs(lines) do
        if string.endswith(line, "\r") then
            if #line > 1 then
                line = string.sub(line, 1, #line - 1)
            else
                line = ""
            end
        end
        if string.find(line, ":") ~= nil then
            if not blitwizard.net.http._header_present(headerblock1,
            line) then
                headerblock1 = headerblock1 .. line .. "\n"
            end
        end
    end
    return headerblock1
end

