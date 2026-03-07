# Root Makefile for personal-blog
#
# This Makefile provides convenience targets for linting all Markdown
# and prose in the repository.

MARKDOWN_FILES := $(shell find . -name '*.md' -not -path './.git/*' -not -path '*/build/*' -not -path '*/builddir/*')

.PHONY: help lint markdownlint codespell

help:
	@echo "Targets:"
	@echo "  lint         - run all linters (markdownlint + codespell)"
	@echo "  markdownlint - lint Markdown files"
	@echo "  codespell    - check spelling in all files"

lint: markdownlint codespell

markdownlint:
	markdownlint $(MARKDOWN_FILES)

codespell:
	codespell
