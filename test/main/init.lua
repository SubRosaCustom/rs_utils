local handlers = {}
local tests_started = false

local function log(message)
	print(string.format("\27[34;1m[rs_utils test]\27[0m %s", message))
end

local function stop(code, message)
	if message then
		log(message)
	end
	os.exit(code)
end

local function protected_call(func)
	local ok, err = pcall(func)
	if not ok then
		stop(1, string.format("\27[31;1m✘\27[0m %s", tostring(err)))
	end
end

function nextTick(func, ticks)
	handlers[#handlers + 1] = {
		func = func,
		ticks = ticks or 1,
	}
end

local function require_test(name)
	log(string.format("Starting %s", name))
	require(name)()
end

local function run_tests()
	require_test("tests.miniz")
	require_test("tests.native")
	require_test("tests.udp")
end

hook.enable("Logic")
hook.enable("PacketReceive")

function hook.run(event)
	if event == "PacketReceive" and testPacketReceive then
		local should_override = false
		protected_call(function()
			should_override = testPacketReceive() == true
		end)
		return should_override
	end

	if event ~= "Logic" then
		return false
	end

	if not tests_started then
		tests_started = true
		protected_call(run_tests)
		return false
	end

	for index = #handlers, 1, -1 do
		local handler = handlers[index]
		handler.ticks = handler.ticks - 1
		if handler.ticks <= 0 then
			table.remove(handlers, index)
			protected_call(handler.func)
		end
	end

	if #handlers == 0 then
		stop(0, "\27[32;1m✔\27[0m All tests passed")
	end

	return false
end
