noinst_DATA= pml2s.jar

if JAVAC_OK
pml2s.jar: always_compile
	cd ../nips_compiler && \
	if ! [ -e lib ]; then mkdir lib; fi && \
	if [ -e lib/pml2s.jar ]; then rm -f lib/pml2s.jar; fi && \
	if [ -e lib/nips-promela-compiler.jar ]; then rm -f lib/nips-promela-compiler.jar; fi && \
	cp ../jar/*.jar ./lib && \
	$(ANT) && \
	mv lib/nips-promela-compiler.jar ../jar/pml2s.jar && \
	rm -rf lib

.PHONY: clean
clean:
	if [ -e pml2s.jar ]; then rm pml2s.jar; fi
	cd ../nips_compiler && \
	if ! [ -e lib ]; then mkdir lib; fi && \
	cp ../jar/*.jar ./lib && \
	ant clean && \
	rm -rf lib

.PHONY: always_compile
always_compile:

endif



