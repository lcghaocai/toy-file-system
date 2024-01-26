
.PHONY: make_all
make_all:
	make step1 && make step2 && make step3

.PHONY: step1 
Copy:
	cd step1 && make

.PHONY: step2
shell:
	cd step2 && make

.PHONY: step3
matrixmul:
	cd step3 && make
