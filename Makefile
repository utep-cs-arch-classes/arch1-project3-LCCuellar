all:
	(cd timerLib; make install)
	(cd lcdLib; make install)
	(cd movShapeLib; make install)
	(cd shapeLib; make install)
	(cd circleLib; make install)
	(cd pacCircleLib; make install)
	(cd p2swLib; make install)
	(cd shape-motion-demo; make)
	(cd testMov; make)
	(cd collision; make)
	(cd project; make)

doc:
	rm -rf doxygen_docs
	doxygen Doxyfile
clean:
	(cd timerLib; make clean)
	(cd lcdLib; make clean)
	(cd movShapeLib; make clean)
	(cd shapeLib; make clean)
	(cd p2swLib; make clean)
	(cd shape-motion-demo; make clean)
	(cd testMov; make clean)
	(cd circleLib; make clean)
	(cd collision; make clean)
	(cd project; make clean)
	(cd pacCircleLib; make clean)
	rm -rf lib h
	rm -rf doxygen_docs/*
