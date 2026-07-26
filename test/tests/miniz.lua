return function()
	local library = require("libminiz")
	assert(library == miniz)
	assert(type(miniz.createZip) == "function")
	assert(type(miniz.extractZip) == "function")

	local expected = {
		["scripts/test.lua"] = "print('hello')",
		["assets/empty.bin"] = "",
		["assets/binary.bin"] = "\0\1\127\128\255",
	}
	local archive = miniz.createZip(expected)
	assert(type(archive) == "string" and #archive > 0)

	local actual = miniz.extractZip(archive)
	for path, bytes in pairs(expected) do
		assert(actual[path] == bytes)
	end

	local ok = pcall(miniz.extractZip, "not a zip archive")
	assert(not ok)
end
