@echo OFF

pushd %~dp0\..\..\..\..
for /F %%A in ('git rev-list HEAD --max-count=1 -- tools/buildsteps/windows/ tools/depends/native/JsonSchemaBuilder/ tools/depends/native/TexturePacker/ tools/depends/target/ffmpeg/FFMPEG-VERSION tools/windows/depends/') do set hashStr=%%A
echo %hashStr%	%NATIVE_PLATFORM%	%TARGET_PLATFORM%	%TARGET_CMAKE_GENERATOR%	%UCRTVersion%
popd
