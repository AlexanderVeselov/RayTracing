fn WriteFile =
(	
	filename = GetSaveFileName types: "Wavefront OBJ (*.OBJ) |*.obj|"
	if filename == undefined then
	(
		return()
	)
	outputFile = CreateFile filename

	rollout progressDialog "Exporting..." width:256 height:56
	(
		progressBar 'pb' "ProgressBar" pos:[0,0] width:253 height:24 color:(color 200 200 200) align:#left
		label 'lbl' "" pos:[8,32] width:240 height:15 enabled:true align:#left
	)
	CreateDialog progressDialog

	Format "# 3ds Max Wavefront OBJ Exporter by Alexander Veselov\n" to:outputFile
	Format "# File Created: %\n\n\n\n" localTime to:outputFile
	for obj in Geometry do
	(
		tmesh = obj.mesh
		mtl = obj.material
		
	-- 	print ("Material: "+mtl.diffuseMap.filename)
	-- 	print mtl.diffuse
		
		Format "#\n# object %\n#\n\n" obj.name to:outputFile
		
		progressDialog.lbl.caption = "Writing vertices..."
		for i = 1 to tmesh.numverts do
		(
			v = GetVert tmesh i
			Format "v  % % %\n" (FormattedPrint v.x Format: ".4f"
				) (FormattedPrint v.y Format: ".4f") (FormattedPrint v.z Format: ".4f") to:outputFile
				
			progressDialog.pb.value = i * 100.0 / tmesh.numverts
		)
		Format "# % vertices\n\n" tmesh.numverts to:outputFile

		progressDialog.lbl.caption = "Writing normals..."
		for f = 1 to tmesh.numfaces do
		(
			normals = meshop.GetFaceRNormals tmesh f
			for i = 1 to 3 do
			(
				n = normals[i]
				Format "vn % % %\n" (FormattedPrint n.x Format: ".4f"
				) (FormattedPrint n.y Format: ".4f") (FormattedPrint n.z Format: ".4f") to:outputFile
			)
			progressDialog.pb.value = f * 100.0 / tmesh.numfaces
		)
		Format "# % vertex normals\n\n" (tmesh.numfaces * 3) to:outputFile

		progressDialog.lbl.caption = "Writing texcoords..."
		numTexCoords = GetNumTVerts tmesh
		for i = 1 to numTexCoords do
		(
			vt = GetTVert tmesh i
			Format "vt % % %\n" (FormattedPrint vt.x Format: ".4f"
				) (FormattedPrint vt.y Format: ".4f") (FormattedPrint vt.z Format: ".4f") to:outputFile
			progressDialog.pb.value = i * 100.0 / numTexCoords
		)
		Format "# % texture coords\n\n" numTexCoords to:outputFile
		
		progressDialog.lbl.caption = "Writing faces..."
		Format "g %\n" (substituteString obj.name " " "_") to:outputFile
		
		if obj.material != undefined then Format "usemtl %" (substituteString obj.material.name " " "_") to:outputFile
		Format "\n" to:outputFile

		nIndex = 1
		for f = 1 to tmesh.numfaces do
		(
			face = GetFace tmesh f
			tface = GetTVFace tmesh f
			
			f1 = face[1] as integer
			f2 = face[2] as integer
			f3 = face[3] as integer
			
			tf1 = tface[1] as integer
			tf2 = tface[2] as integer
			tf3 = tface[3] as integer
			
			Format "f %/%/% %/%/% %/%/%\n" f1 tf1 nIndex f2 tf2 (nIndex+1) f3 tf3 (nIndex+2) to:outputFile
			nIndex = nIndex + 3
			progressDialog.pb.value = f * 100.0 / tmesh.numfaces
	-- 		matid = GetFaceMatId tmesh f
		)
		Format "# % faces\n\n" tmesh.numfaces to:outputFile
		
	)
	
	close outputFile
	
	progressDialog.lbl.caption = "Export complete"	
	DestroyDialog progressDialog
)

rollout ExportMesh "Export OBJ" width:256 height:80
(
	button 'btn2' "Export" pos:[56,32] width:144 height:32 align:#left
	checkbox 'chk1' "Export Materials" pos:[8,8] width:144 height:16 checked:true align:#left
	
	on btn2 pressed do
	(
		WriteFile()
	)
)
CreateDialog ExportMesh



-- rollout ExportMesh "Export Mesh" width:256 height:72
-- (
-- 	button 'btn2' "Export" pos:[56,32] width:144 height:32 align:#left
-- 	checkbox 'chk1' "Export Materials" pos:[8,8] width:144 height:16 checked:true align:#left
-- 	
-- 	on btn2 pressed do
-- 	(
-- 		WriteFile()
-- 	)
-- )

-- RF = NewRolloutFloater "Obj Exporter" 270 300
-- AddRollout exportMesh 