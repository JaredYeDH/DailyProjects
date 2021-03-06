dofile('fp.lua')
------------------------------
function async(f)
    return function(...)
        local task = {}
        local fargs = arg
        task._callback = fargs[#fargs]
        fargs[#fargs] = task

        task.await = function(op, ...)
            arg[#arg + 1] = function(value)
                coroutine.resume(task._coroutine, value)
            end
            op(unpack(arg))
            return coroutine.yield()
        end

        task._coroutine = coroutine.create(function() 
            task._callback(f(unpack(fargs)))
        end)
        coroutine.resume(task._coroutine)
    end
end
------------------------------
function whenAll(tasks, callback)
    local rest = #tasks
    for _, task in ipairs(tasks) do
        task(function(v) 
            rest = rest - 1
            if rest == 0 then
                callback(nil)
            end
        end)
    end
end
------------------------------
local _events = {}
local _now = 0
function time()
    return _now
end
function insertEvent(delay, callback)
    local finishTime = time() + delay
    local i = 1
    while i <= #_events and _events[i][1] <= finishTime do
        i = i + 1
    end
    table.insert(_events, i, {finishTime, callback})
end 
function dispatchEvent()
    local event = _events[1]
    if not event then return end
    table.remove(_events, 1)
    _now = event[1]
    event[2]()
    return event
end
function connect(url, port, callback)
    insertEvent(math.random(5, 10), function() callback({}) end)
end
function send(conn, data, callback)
end
function receive(conn, callback)
    insertEvent(math.random(1, 4), function() callback('response: ' .. time()) end)
end
function close(conn)
end
------------------------------
local requestUrlAsync = async(function(url, port, task)
    local conn = task.await(connect, url, port)
    print(string.format('[%d] %s: connected...', time(), url))

    send(conn, 'GET / HTTP/1.0')
    for i = 1, 10 do
        local resp = task.await(receive, conn)
        print(string.format('[%d] %s: receive tmp...', time(), url))
        send(conn, resp)
    end

    local html = task.await(receive, conn)
    close(conn)
    print(string.format('[%d] %s: close', time(), url))

    return html
end)
------------------------------
local requestUrlsSync = async(function(urls, task)
    for _, url in ipairs(urls) do
        task.await(requestUrlAsync, url, 80)
    end
end)
local requestUrlsAsync = async(function(urls, task)
    task.await(whenAll, {mapa(function(url) 
        return function(callback)
            requestUrlAsync(url, 80, callback)
        end
    end, unpack(urls))})
end)
------------------------------
math.randomseed(os.time())

--requestUrlsSync(
requestUrlsAsync(
    {'www.sina.com', 'www.baidu.com', 'www.taobao.com', 'www.qq.com'}, function() print('all finished') end)
while dispatchEvent() do end
