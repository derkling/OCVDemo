
ifdef CONFIG_CONTRIB_OCVDEMO

# Targets provided by this project
.PHONY: ocvdemo clean_ocvdemo

# Add this to the "contrib_testing" target
contrib: ocvdemo
clean_contrib: clean_ocvdemo


ocvdemo: external
	@echo
	@echo "==== Building OCVDemo ($(BUILD_TYPE)) ===="
	@[ -d contrib/ocvdemo/build/$(BUILD_TYPE) ] || \
		mkdir -p contrib/ocvdemo/build/$(BUILD_TYPE) || \
		exit 1
	@cd contrib/ocvdemo/build/$(BUILD_TYPE) && \
		CXX=$(CXX) cmake $(CMAKE_COMMON_OPTIONS) || \
		exit 1
	@cd contrib/ocvdemo/build/$(BUILD_TYPE) && \
		make -j$(CPUS) install || \
		exit 1

clean_ocvdemo:
	@echo
	@echo "==== Clean-up OCVDemo ===="
	@[ ! -f $(BUILD_DIR)/usr/bin/bbque-ocvdemo ] || \
		rm -f $(BUILD_DIR)/etc/bbque/recipes/OCVDemo*; \
		rm -f $(BUILD_DIR)/usr/bin/bbque-ocvdemo*
	@rm -rf contrib/ocvdemo/build
	@echo

else # CONFIG_CONTRIB_OCVDEMO

ocvdemo:
	$(warning contib/bbque-ocvdemo module disabled by BOSP configuration)
	$(error BOSP compilation failed)

endif # CONFIG_CONTRIB_OCVDEMO

