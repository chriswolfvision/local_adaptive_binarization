

all:
	g++ -I/usr/include/opencv binarizewolfjolion.cpp -o binarizewolfjolion `pkg-config opencv --libs`-lstdc++

clean:
	rm -f binarizewolfjolion

test:
	./binarizewolfjolion -k 0.6 sample.jpg _result.jpg


package:	clean
	rm -f x.jpg
	tar cvfz binarizewolfjolionopencv.tgz *

