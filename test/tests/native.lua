return function()
	local native = require("librosaserver_src_integration")
	assert(native == srcIntegrationNative)

	for _, name in ipairs({
		"loadITM",
		"loadIT3",
		"loadSBV",
		"clearCustomVehicleTypeSlots",
		"setupVehicleTypeNew",
		"setupObjectTypeWeight",
		"randomToken",
		"sendPacket",
		"drainSrcPackets",
		"currentPacketEndpoint",
	}) do
		assert(type(native[name]) == "function", name)
	end

	assert(itemTypes.getCount() >= 46)
	assert(#itemTypes == itemTypes.getCount())
	assert(#itemTypes.getAll() == itemTypes.getCount())
	assert(itemTypes[0])
	assert(not pcall(function()
		return itemTypes[256]
	end))

	assert(vehicleTypes.getCount() >= 17)
	assert(#vehicleTypes == vehicleTypes.getCount())
	assert(#vehicleTypes.getAll() == vehicleTypes.getCount())
	assert(vehicleTypes[0])
	assert(not pcall(function()
		return vehicleTypes[128]
	end))

	local tokens = {}
	for _ = 1, 16 do
		local token = native.randomToken()
		assert(token:match("^[0-9a-f]+$") and #token == 32)
		assert(not tokens[token])
		tokens[token] = true
	end

	assert(not pcall(native.loadITM, -1, "data/watermelon.itm"))
	assert(not pcall(native.loadIT3, 256, "data/item/tabletest2.it3"))
	assert(not pcall(native.loadSBV, 128, "park5"))
	assert(not pcall(native.setupVehicleTypeNew, 128, 0, 1, 1))
	assert(not pcall(native.setupObjectTypeWeight, 128))
end
