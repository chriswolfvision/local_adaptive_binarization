# Local Adaptive Binarization


This code uses an improved contrast maximization version of Niblack/Sauvola et al's method to binarize document images. It is also able to perform the more classical Niblack as well as Sauvola et al. methods. Details can be found in the [ICPR 2002 paper](file:///Users/chris/www/prof/publications/index.html#icpr2002v).


    You need to cite the following paper when you use this code :

    Christian Wolf, Jean-Michel Jolion and Francoise Chassaing. Text Localization, Enhancement and Binarization in Multimedia Documents. In International Conference on Pattern Recognition (ICPR), volume 4, pages 1037-1040, 2002. 

## Usage:

The executable is called on the command line and only reads and writes PGM files. Under Linux you can use for instance "convert" (part of the ImageMagick package) to convert to and from the PGM format. The first argument chooses between one of several methods, the first and the second argument specify, respectively, the input and the output file:

```
usage: binarize [ -x  -y  -k  ] [ version ]  

version: n   Niblack (1986)         needs white text on black background
         s   Sauvola et al. (1997)  needs black text on white background
         w   Wolf et al. (2001)     needs black text on white background

Default version: w (Wolf et al. 2001)
Default value for "k": 0.5

example:
       binarize w in.pgm out.pgm
       binarize in.pgm out.pgm
       binarize s -x 50 -y 50 -k 0.6 in.pgm out.pgm
```

The best working method is 'w', the one which performed 5th in the [DIBCO 2009 competition](http://www.cvc.uab.es/icdar2009/papers/3725b375.pdf).

If no further arguments are provided, than the window sizes are estimated automatically. The argument -k sets the "k" parameter from all 3 papers. 

    IMPORTANT! Note, that the parameter should be set differently according! It might be necessary to set a different parameter for Niblack's method (he recommends -0.2 in his paper). 




