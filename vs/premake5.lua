--
-- premake5 file to build RecastDemo
-- http://premake.github.io/
--
require "lib"

local action = _ACTION or ""
local outdir = action


WorkSpaceInit  "acc_svr"



-- Project "acc_svr"
	-- IncludeFile { 
		-- "../External/libevent_cpp/External/libevent-2.1.8-stable/out/include/",
		-- "../External/libevent_cpp/include/", --浏览用 cmake不用
		-- "../External/svr_util/include/",--浏览用 cmake不用
		-- "../External/",
	-- }

	-- SrcPath { 
		-- "../acc_svr/**",  --**递归所有子目录，指定目录可用 "cc/*.cpp" 或者  "cc/**.cpp"
	-- }
	-- files {
	-- "../*.txt",
	-- "../*.lua",
	-- "../External/svr_util/include/timer/*.h",
	-- }
	
Project "acc_proto"
	IncludeFile { 
		"../External/svr_util/include/",
	}

	SrcPath { 
		"../acc_proto/**",  
	}	
	files {
	"../acc_proto/*.txt",
	}
	
-- Project "acc_driver"
	-- IncludeFile { 
		-- "../External/svr_util/include/",
	-- }

	-- SrcPath { 
		-- "../mf_driver/**",  
	-- }	
	-- files {
	-- "../mf_driver/*.txt",
	-- }
	
-- Project "samples"
	-- includedirs { 
		-- "../include/",
		-- "base/",
	-- }

	-- SrcPath { 
		-- "../samples/**",  
	-- }
	
-- Project "test_combine"
	-- includedirs { 
		-- "../include/",
	-- }

	-- SrcPath { 
		-- "../test/test_combine/**",  
		-- "../test/com/**",  
	-- }
	
    
    
    