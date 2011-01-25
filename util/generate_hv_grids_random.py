#!/usr/bin/env python
'''Generate box grids of the whole earth for testing'''

import numpy

ew_divisions = 360*4
ns_divisions = 180*4

lons_slice = numpy.linspace(-180.0,180.0, ew_divisions).astype('float32')
lats_slice = numpy.linspace(-90.0,90.0, ns_divisions).astype('float32')

lats_array = numpy.repeat(lats_slice, ew_divisions)
lons_array = numpy.tile(lons_slice, ns_divisions)

data_array = numpy.ndarray(lats_array.shape, 'float32')
data_array[:] = 1.0

lats_array.tofile('lats')
lons_array.tofile('lons')
data_array.tofile('data_file')
