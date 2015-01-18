
function uuid_example()
    saker.log(LOG_INFO, "This is uuid_example")
    local files = saker.walk("..", "*.exe")
    for k,v in pairs(files) do
        saker.log(LOG_INFO, v)
    end
    return true
end

saker.register("uuid_example", "uuid_example", PROP_ONCE)
