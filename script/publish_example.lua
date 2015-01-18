
function publish_example()
    saker.log(LOG_INFO, "This is publish_example")

    saker.log(LOG_INFO, "UUID : "..saker.uuid())
    saker.publish("foo", "UUID : "..saker.uuid())
    return true
end

saker.register("publish_example", "publish_example", PROP_CYCLE)
