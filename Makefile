all:
	c++ -O2 -I/usr/include/opencv4 binarizewolfjolion.cpp -o binarizewolfjolion `pkg-config opencv4 --libs` -lstdc++

test: all
	./binarizewolfjolion -k '-0.2' -m 'n' sample.jpg result_n.jpg
	./binarizewolfjolion -k '0.6'  -m 's' sample.jpg result_s.jpg
	./binarizewolfjolion -k '0.6'  -m 'w' sample.jpg result_w.jpg

clean:
	rm -f binarizewolfjolion
	rm -f result_*.jpg

package: clean
	tar -cvfz binarizewolfjolionopencv.tar.gz *
