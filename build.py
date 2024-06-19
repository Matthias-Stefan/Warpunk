import os
import threading

# Define the options for the compiler and linker
#/std:c++20
CL_OPTIONS = r"/std:c++17 -DWARPUNK_DEBUG=1 -MTd -Od -Z7 -EHsc /nologo /IW:\Warpunk /IW:\VulkanSDK\1.3.280.0\Include"
LK_OPTIONS = r"-opt:ref user32.lib gdi32.lib winmm.lib kernel32.lib ole32.lib /LIBPATH:W:\VulkanSDK\1.3.280.0\Lib vulkan-1.lib"

def compile_file(file, output):
    # Only compile .cpp files
    if file.endswith('.cpp') or file.endswith('.c'):
        os.system(f"cl {CL_OPTIONS} /c {file} /Fo:{output} /Fd:{output}")

def link_files(output, export_functions, *files):
    export_options = " ".join([f"/EXPORT:{func}" for func in export_functions])
    os.system(f"link {LK_OPTIONS} {export_options} /OUT:{output} {' '.join(files)} /DEBUG")

# Delete all .pdb files
os.system("del *.pdb > NUL 2> NUL")

# Create a temporary lock file
with open('lock.tmp', 'w') as f:
    f.write('WAITING FOR PDB')

# List of files to compile:
files = [
    (r".\source\warpunk.cpp", r".\build\objs\warpunk.obj"),
    (r".\source\win32_warpunk.cpp", r".\build\objs\win32_warpunk.obj"),

    (r".\source\platform\mouse.cpp", r".\build\objs\mouse.obj"),
    (r".\source\platform\keyboard.cpp", r".\build\objs\keyboard.obj"),
    # Add more files as needed...
]

# Create threads for each compilation
threads = [threading.Thread(target=compile_file, args=file) for file in files]

# Start all threads
for thread in threads:
    thread.start()

# Wait for all threads to finish
for thread in threads:
    thread.join()

# Define export functions
export_functions = [
    "GameUpdateAndRender",
    "GameInitialize"
    # Add more export functions as needed...
]

# Output directory for the DLL
output_dir = ".\\build"

# Create output directory if it doesn't exist
os.makedirs(output_dir, exist_ok=True)

# Link all compiled files into a DLL
link_files(os.path.join(output_dir, "warpunk.dll"), export_functions, *[file[1] for file in files])

# Link all compiled files into an executable
link_files(os.path.join(output_dir, "warpunk.exe"), [], *[file[1] for file in files])

# Remove temporary lock file
os.remove('lock.tmp')
