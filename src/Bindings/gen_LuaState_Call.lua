
-- gen_LuaState_Call.lua

-- Generates the cLuaState::Call() function templates that are included from LuaState.h

--[[
The cLuaState::Call() family of functions provides a template-based system for calling any Lua function
either by name or by reference with almost any number of parameters and return values. This is done by
providing a number of overloads of the same name with variable number of template-type parameters. To
separate the arguments from the return values, a special type of cLuaState::cRet is used.
--]]




print("Generating LuaState_Call.inc . . .")




-- List of combinations (# params, # returns) to generate:
local Combinations =
{
	-- no return values:
	{0, 0},
	{1, 0},
	{2, 0},
	{3, 0},
	{4, 0},

	-- 1 return value:
	{0, 1},
	{1, 1},
	{2, 1},
	{3, 1},
	{4, 1},
	{5, 1},
	{6, 1},
	{7, 1},
	{8, 1},
	{9, 1},
	{10, 1},

	-- 2 return values:
	{0, 2},
	{1, 2},
	{2, 2},
	{3, 2},
	{4, 2},
	{5, 2},
	{6, 2},
	{7, 2},
	{8, 2},
	{9, 2},
	
	-- Special combinations:
	{7, 3},
	{8, 3},
	{9, 5},
}




--- Writes a single overloaded function definition for the specified number of params and returns into f
--[[
The format for the generated function is this:
/** Call the specified 3-param 2-return Lua function:
Returns true if call succeeded, false if there was an error. */
template <typename FnT, typename ParamT1, typename ParamT2, typename ParamT3, typename RetT1, typename RetT2>
bool Call(FnT a_Function, ParamT1 a_Param1, ParamT2 a_Param2, ParamT3 a_Param3, const cLuaState::cRet & a_RetMark, RetT1 & a_Ret1, RetT2 & a_Ret2)
{
	UNUSED(a_RetMark);
	if (!PushFunction(a_Function))
	{
		return false;
	}
	Push(a_Param1);
	Push(a_Param2);
	Push(a_Param3);
	if (!CallFunction(2))
	{
		return false;
	}
	GetStackValue(-2, a_Ret1);
	GetStackValue(-1, a_Ret2);
	lua_pop(m_LuaState, 2);
	return true;
}
Note especially the negative numbers in GetStackValue() calls.
--]]
local function WriteOverload(f, a_NumParams, a_NumReturns)
	-- Write the function doxy-comments:
	f:write("/** Call the specified ", a_NumParams, "-param ", a_NumReturns, "-return Lua function:\n")
	f:write("Returns true if call succeeded, false if there was an error. */\n")
	
	-- Write the template <...> line:
	f:write("template <typename FnT")
	for i = 1, a_NumParams do
		f:write(", typename ParamT", i)
	end
	if (a_NumReturns > 0) then
		for i = 1, a_NumReturns do
			f:write(", typename RetT", i)
		end
	end
	f:write(">\n")
	
	-- Write the function signature:
	f:write("bool Call(")
	f:write("FnT a_Function")
	for i = 1, a_NumParams do
		f:write(", ParamT", i, " a_Param", i)
	end
	if (a_NumReturns > 0) then
		f:write(", const cLuaState::cRet & a_RetMark")
		for i = 1, a_NumReturns do
			f:write(", RetT", i, " & a_Ret", i)
		end
	end
	f:write(")\n")
	
	-- Common code:
	f:write("{\n")
	if (a_NumReturns > 0) then
		f:write("\tUNUSED(a_RetMark);\n")
	end
	f:write("\tif (!PushFunction(a_Function))\n")
	f:write("\t{\n")
	f:write("\t\treturn false;\n")
	f:write("\t}\n")
	
	-- Push the params:
	for i = 1, a_NumParams do
		f:write("\tPush(a_Param", i, ");\n")
	end
	
	-- Call the function:
	f:write("\tif (!CallFunction(", a_NumReturns, "))\n")
	f:write("\t{\n")
	f:write("\t\treturn false;\n")
	f:write("\t}\n")
	
	-- Get the return values:
	for i = 1, a_NumReturns do
		f:write("\tGetStackValue(", -1 - a_NumReturns + i, ", a_Ret", i, ");\n")
	end
	
	-- Pop the returns off the stack, if needed:
	if (a_NumReturns > 0) then
		f:write("\tlua_pop(m_LuaState, ", a_NumReturns, ");\n")
	end
	
	-- Everything ok:
	f:write("\treturn true;\n")
	f:write("}\n")
	
	-- Separate from the next function:
	f:write("\n\n\n\n\n")
end





local f = assert(io.open("LuaState_Call.inc", "w"))

-- Write file header:
f:write([[
// LuaState_Call.inc

// This file is auto-generated by gen_LuaState_Call.lua
// Make changes to the generator instead of to this file!

// This file contains the various overloads for the cLuaState::Call() function
// Each overload handles a different number of parameters / return values
]])
f:write("\n\n\n\n\n")

-- Write out a template function for each overload:
for _, combination in ipairs(Combinations) do
	WriteOverload(f, combination[1], combination[2])
end

-- Close the generated file
f:close()





print("LuaState_Call.inc generated.")




