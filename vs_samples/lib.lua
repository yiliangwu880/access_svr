--lib

--cpp项目缺省扩展名
DEF_CPP_EXT_LS={"cpp","cc","h", "hpp", "txt", "lua"}




local action = _ACTION or ""
local outdir = action

function WorkSpaceInit(name)
	workspace(name)
		configurations { 
			"Debug",
			--"Release"
		}
		location (outdir)
end	
	
--路径列表加入扩展名列表 
--比如：
--[[ 
path_list={ "../lib_prj/**"}
ext_ls={"cpp", "cc"}
path_list =BuildFileExtenal(path_list, ext_ls)后
结果 path_list={ 
"../lib_prj/**.cpp"
,"../lib_prj/**.cc"
}
--]]		
function BuildFileExtenal(path_list, ext_ls)
	if nil == ext_ls then
		ext_ls = DEF_CPP_EXT_LS
	end
	ret={}
	for _,ext in pairs(ext_ls) do
		for _,path in pairs(path_list) do
			s =path.."."..ext
			table.insert(ret, s)
		end	
	end	
	return ret
end

--加入源文件。 
--@para path_list 路径列表 格式{ "../lib_prj/**", "../lib_prj2/**"}
--@para ext_ls 扩展名列表，格式{"cpp", "cc"}
function SrcPath(path_list, ext_ls)
	path_list=BuildFileExtenal(path_list, ext_ls)
	files(path_list) --系统函数
end

function Project(project_name)
	project(project_name)
	language "C++"
	kind "ConsoleApp"
end

--路径列表加入扩展名列表 
--比如：
--[[ 
path_list={ "../lib_prj/"}
ext_ls={"*.cpp", "*.cc"}
path_list =BuildFileExtenal(path_list, ext_ls)后
结果 path_list={ 
"../lib_prj/*.cpp"
,"../lib_prj/*.cc"
}
--]]		
function BuildFileExtenal2(path_list, ext_ls)
	if nil == ext_ls then
		ext_ls = DEF_CPP_EXT_LS
	end
	ret={}
	for _,ext in pairs(ext_ls) do
		for _,path in pairs(path_list) do
			s =path.."*."..ext
			--print("path2="..s)
			table.insert(ret, s)
		end	
	end	
	return ret
end

--加入包含路径。 
--@para path_list 路径列表 格式{ "../lib_prj/", "../lib_prj2/"}
function IncludeFile(path_list)
	includedirs(path_list)
	path_list=BuildFileExtenal2(path_list, {"*.h", "*.hpp"})
	--print(path_list[1])
	files(path_list)
end