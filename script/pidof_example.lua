
function pidof_example()
    local pid, err = saker.pidof("/var/run/nginx.pid", "*nginx*")
    if pid ~= nil then 
      print("nginx pid is "..pid)
    end
    return false, err
end

--saker.register("pidof_example", "pidof_example", PROP_CYCLE)
