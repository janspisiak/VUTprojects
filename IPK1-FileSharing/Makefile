GMAKE=`command -v gmake >/dev/null 2>&1 && { echo "gmake"; true; } || echo "make"`
USEGNU=$(GMAKE) -f GNUMakefile $@

release:
	@$(USEGNU)
.DEFAULT:
	@$(USEGNU)
.PHONY: release