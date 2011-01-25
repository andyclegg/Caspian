#!/usr/bin/env python
'''Generate box grids of the whole earth for testing'''

import numpy

ew_divisions = 18
ns_divisions = 9

ns_marks = numpy.asarray([-90 + i * (180 / ns_divisions) for i in range(ns_divisions+1)], 'float32')
ew_marks = numpy.asarray([-180 + i * (360 / ew_divisions) for i in range(ew_divisions+1)], 'float32')

lons_slice = numpy.linspace(-180.0,180.0, 1000).astype('float32')
lats_slice = numpy.linspace(-90.0,90.0, 1000).astype('float32')

vertical_lats_array = numpy.tile(lats_slice, len(ew_marks))
vertical_lons_array = numpy.repeat(ew_marks, 1000)

horizontal_lats_array = numpy.repeat(ns_marks, 1000)
horizontal_lons_array = numpy.tile(lons_slice, len(ns_marks))

combined_lats_array = numpy.concatenate((vertical_lats_array, horizontal_lats_array))
combined_lons_array = numpy.concatenate((vertical_lons_array, horizontal_lons_array))

data_array = numpy.ndarray(combined_lats_array.shape, 'float32')
data_array[:] = 1.0

combined_lats_array.tofile('lats')
combined_lons_array.tofile('lons')
data_array.tofile('data_file')
