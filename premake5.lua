local workspaceName = "workspace"
local projectName   = "project"

workspace(workspaceName)

configurations {
        "debug",
        "release"
}

project (projectName)
    language "C++"   
    targetdir "bin/%{cfg.buildcfg}"
    objdir "obj/%{cfg.buildcfg}"
    cppdialect "C++20"
    cdialect("C11")
    staticruntime "on"


    links { 
        --"glfw",
        --"vulkan",
        "X11"
    }

    files { 
        "src/**.h",
        "src/**.hpp",
        "src/**.c",
        "src/**.cpp"
    }

    includedirs { 
        "src/"
    }


    filter "configurations:debug"   
    defines { 
        "DEBUG",
        "_DEBUG"
    }
        optimize "Debug"
        symbols "On"
        kind "ConsoleApp"   


    filter "configurations:release"      
        defines { "NDEBUG"}
            
        optimize "Full"
        symbols "Off"
        kind "ConsoleApp"   