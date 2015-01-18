
function uuid_example()
  saker.log(LOG_INFO, "This is uuid_example")
  saker.log(LOG_INFO, "UUID : "..saker.uuid())
  return true
end

saker.register("uuid_example", "uuid_example", PROP_ONCE)
