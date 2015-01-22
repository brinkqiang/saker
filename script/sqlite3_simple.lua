-- example for use sqlite3
function sqlite3_example()
    local db = sqlite3.open_memory()

    db:exec[[
      CREATE TABLE test (id INTEGER PRIMARY KEY, content);

      INSERT INTO test VALUES (NULL, 'Hello World');
      INSERT INTO test VALUES (NULL, 'Hello Lua');
      INSERT INTO test VALUES (NULL, 'Hello Sqlite3')
    ]]
    for row in db:nrows("SELECT * FROM test") do
      print(row.id, row.content)
    end
    return true
end

saker.register("sqlite3_example", "sqlite3_example", PROP_CYCLE)


