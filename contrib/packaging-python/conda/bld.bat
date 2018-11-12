REM To avoid building in work/ alongside the source. Rather build in work/build/
mkdir build
cd build

REM Remove dot from PY_VER for use in library name
set MY_PY_VER=%PY_VER:.=%

REM Configure step
cmake -G "%CMAKE_GENERATOR%" ^
 -DCMAKE_INSTALL_PREFIX="%PREFIX%" ^
 -DPYTHON_EXECUTABLE:FILEPATH="%PYTHON%" ^
 -DPYTHON_INCLUDE_DIR:PATH="%PREFIX%"/include ^
 -DPYTHON_LIBRARY:FILEPATH="%PREFIX%"/libs/python%MY_PY_VER%.lib ^
 --config "%CONFIGURATION%" ^
 -DENABLE_MODULE_IRRLICHT=ON ^
 -DENABLE_MODULE_FEA=OFF ^
 -DENABLE_MODULE_POSTPROCESS=OFF ^
 -DENABLE_MODULE_PYTHON=ON ^
 -DBUILD_DEMOS=OFF ^
 -DBUILD_TESTING=OFF ^
 -DCH_IRRLICHTDIR="C:\irrlicht-1.8.2" ^
 -DCH_IRRLICHTLIB="C:\irrlicht-1.8.2\lib\Win64-visualStudio\Irrlicht.lib" ^
 ..
if errorlevel 1 exit 1
 
REM Build step 
cmake --build . --config "%CONFIGURATION%"
if errorlevel 1 exit 1

REM Install step 
cmake --build . --config "%CONFIGURATION%" --target install
if errorlevel 1 exit 1

REM Install step
REM ninja install
REM if errorlevel 1 exit 1

