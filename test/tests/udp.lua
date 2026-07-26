local function read_file(path)
	local file = io.open(path, "rb")
	if not file then
		return nil
	end
	local bytes = file:read("*all")
	file:close()
	return bytes
end

local function write_file(path, bytes)
	local file = assert(io.open(path, "wb"))
	file:write(bytes)
	file:close()
end

return function()
	local native = assert(srcIntegrationNative)
	local expected_outbound = "rs-utils-outbound\0\255"
	local expected_inbound = "7DFPSRCUrs-utils-inbound\0\255"
	local attempts = 0

	assert(not pcall(native.sendPacket, "127.0.0.1", 0, "x"))
	assert(not pcall(native.sendPacket, "127.0.0.1", 65536, "x"))
	assert(not pcall(native.sendPacket, "127.0.0.1", 27061, ""))
	assert(not pcall(native.sendPacket, "127.0.0.1", 27061, string.rep("x", 1201)))

	local function wait_for_probe()
		attempts = attempts + 1
		if not read_file("udp_probe.ready") then
			assert(attempts < 600, "UDP probe did not become ready")
			nextTick(wait_for_probe)
			return
		end

		assert(native.sendPacket("127.0.0.1", 27061, expected_outbound) == #expected_outbound)
		write_file("udp_inbound.ready", "ready")
	end
	nextTick(wait_for_probe)

	testPacketReceive = function()
		local result = native.drainSrcPackets()
		for _, packet in ipairs(result.packets) do
			if packet.data == expected_inbound then
				assert(packet.address == "127.0.0.1")
				assert(packet.port == 27061)
				testInboundPacket = packet.data
			end
		end
		return result.drained > 0 and result.vanillaPending ~= true
	end

	local result_attempts = 0
	local function wait_for_results()
		result_attempts = result_attempts + 1
		local outbound = read_file("udp_outbound.bin")
		if outbound ~= expected_outbound or testInboundPacket ~= expected_inbound then
			assert(result_attempts < 600, "UDP loopback did not complete")
			nextTick(wait_for_results)
			return
		end
		testPacketReceive = nil
	end
	nextTick(wait_for_results)
end
