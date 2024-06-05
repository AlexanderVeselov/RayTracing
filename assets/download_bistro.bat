mkdir Bistro
pushd Bistro
powershell Invoke-WebRequest -OutFile BuildingTextures.zip -Uri https://casual-effects.com/g3d/data10/research/model/bistro/BuildingTextures
powershell Invoke-WebRequest -OutFile OtherTextures.zip -Uri https://casual-effects.com/g3d/data10/research/model/bistro/OtherTextures
powershell Invoke-WebRequest -OutFile PropTextures.zip -Uri https://casual-effects.com/g3d/data10/research/model/bistro/PropTextures
powershell Invoke-WebRequest -OutFile Exterior.zip -Uri https://casual-effects.com/g3d/data10/research/model/bistro/Exterior.zip

powershell Expand-Archive BuildingTextures.zip
powershell Expand-Archive OtherTextures.zip
powershell Expand-Archive PropTextures.zip
powershell Expand-Archive Exterior.zip

rm BuildingTextures.zip
rm OtherTextures.zip
rm PropTextures.zip
rm Exterior.zip

popd
