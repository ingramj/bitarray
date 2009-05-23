require 'bitfield'
require 'bitarray'
require 'benchmark'

Benchmark.bm(24) { |bm|
  puts "------------------------ Object instantiation."
  bm.report("BitField initialize") { 10000.times { BitField.new(256) } }
  bm.report("BitArray initialize") { 10000.times { BitArray.new(256) } }

  bf = BitField.new(256)
  ba = BitArray.new(256)
  
  puts "------------------------ Element Reading"
  bm.report("BitField []")         { 10000.times { bf[rand(256)] } }
  bm.report("BitArray []")         { 10000.times { ba[rand(256)] } }

  puts "------------------------ Element Writing"
  bm.report("BitField []=")        { 10000.times { bf[rand(256)] = [0,1][rand(2)] } }
  bm.report("BitArray []=")        { 10000.times { ba[rand(256)] = [0,1][rand(2)] } }

  puts "------------------------ Element Enumeration"
  bm.report("BitField each")       { 10000.times { bf.each {|b| b } } }
  bm.report("BitArray each")       { 10000.times { ba.each {|b| b } } }

  puts "------------------------ To String"
  bm.report("BitField to_s")       { 10000.times { bf.to_s } }
  bm.report("BitArray to_s")       { 10000.times { ba.to_s } }

  puts "------------------------ BitArray methods"
  bm.report("BitArray set_all_bits")    { 10000.times { ba.set_all_bits} }
  bm.report("BitArray clear_all_bits")  { 10000.times { ba.clear_all_bits } }
  bm.report("BitArray toggle_bit")      { 10000.times { ba.toggle_bit(1) } }
  bm.report("BitArray toggle_all_bits") { 10000.times { ba.toggle_all_bits } }
  bm.report("BitArray clone")           { 10000.times { ba.clone } }
}

