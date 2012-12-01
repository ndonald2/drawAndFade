# openFrameworks install script
cp -f ../../../libs/fmodex/lib/osx/libfmodex.dylib "$TARGET_BUILD_DIR/$PRODUCT_NAME.app/Contents/MacOS/libfmodex.dylib"; install_name_tool -change ./libfmodex.dylib @executable_path/libfmodex.dylib "$TARGET_BUILD_DIR/$PRODUCT_NAME.app/Contents/MacOS/$PRODUCT_NAME";


# copy openni libraries
echo ""
echo "======================================"
echo "Copying openni libraries to bundle...."
echo "======================================"
echo ""
cd bin/data/openni/lib

libs=(*.dylib)
echo ${libs[@]}

for lib in *.dylib ; do

    echo ${lib}
    cp -f ${lib} "$TARGET_BUILD_DIR/$PRODUCT_NAME.app/Contents/MacOS/${lib}"
    echo cp -f ${lib} "$TARGET_BUILD_DIR/$PRODUCT_NAME.app/Contents/MacOS/${lib}"

done

# change library-to-executable install names
echo ""
echo "======================================"
echo "Changing openni library install names...."
echo "======================================"
echo ""
cd $TARGET_BUILD_DIR/$PRODUCT_NAME.app/Contents/MacOS

lines=`otool -L $PRODUCT_NAME`
echo ${lines}

for linked_lib in ${lines} ; do

	libname=`basename ${linked_lib}`

	check_lib=`echo ${libs[@]} | grep -i ${libname}`

	if [ "$check_lib" != "" ] ; then
		echo "---------------------"
		echo "$libname"
		install_name_tool -change ${linked_lib} "@executable_path/${libname}" $PRODUCT_NAME
		echo install_name_tool -change ${linked_lib} "@executable_path/${libname}" $PRODUCT_NAME

	fi
done

# change library-to-library install names
echo ""
echo "======================================"
echo "Changing inter-library install names...."
echo "======================================"
echo ""

for openni_lib in ${libs[@]} ; do

	echo "---------------------"
	echo "${openni_lib}"

	liblibs=`otool -L ${openni_lib}`
	echo ${liblibs}

	for liblib in ${liblibs[@]} ; do
	
		libname=`basename ${liblib}`
		check_lib=`echo ${libs[@]} | grep -i ${libname}`
		if [ "$check_lib" != "" ] && [ "$libname" != "$openni_lib" ] ; then

			install_name_tool -change ${liblib} "@executable_path/${libname}" ${openni_lib}
			echo install_name_tool -change ${liblib} "@executable_path/${libname}" ${openni_lib}

		fi

	done

done
