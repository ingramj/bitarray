#!/usr/bin/env ruby
require 'digest/sha1'
require 'bitarray'

# This bloom filter was written as a demonstration of the BitArray class.
# Therefore, it was written with an eye towards simplicity rather than
# efficiency or optimization.
#
# For information about picking parameters for a Bloom Filter, take a look at
# the Wikipedia page.
#
# http://en.wikipedia.org/wiki/Bloom_filter
#
class BloomFilter
  def initialize(m = 1000000, k = 3)
    @size = m
    @hashes = k < 3 ? 3 : k
    @ba = BitArray.new(@size)
  end

  def add(input)
    hash(input).each {|i|
      @ba.set_bit i
    }
  end

  def include?(input)
    hash(input).each {|i|
      return false if @ba[i] == 0
    }
    return true
  end

  private
  # Return an array of @hashes indices to set. 
  #
  # We generate as many hash values as needed by using the technique described
  # by Kirsch and Mitzenmacher[1].
  #
  # [1] http://www.eecs.harvard.edu/~kirsch/pubs/bbbf/esa06.pdf
  def hash(input)
    h1 = input.hash.abs % @size
    h2 = Digest::SHA1.hexdigest(input).to_i(16) % @size

    ha = [h1, h2]
    1.upto(@hashes - 2) do |i|
      ha << (h1 + i * h2) % @size
    end
    return ha
  end
end


# As a demonstration, load the contents of the dictionary file and let the user
# look up words. 
#
# Using the calculator at http://hur.st/bloomfilter, the optimum number of bits
# for my system's dictionary file (98569 words), with a false positive rate of
# 0.001, is 1,417,185 bits (about 173 Kb), and 10 hash functions.
#
# Loading the dictionary takes about 4 seconds on my system.
if __FILE__ == $0
  print "Loading dictionary..."
  bf = BloomFilter.new(1417185, 10)
  File.open('/usr/share/dict/words') {|f|
    f.each_line {|w| bf.add(w.chomp)}
  }
  print "done\n"

  puts "Enter words to look up, ctrl-d to quit."
  done = false
  while (!done)
    print "Word: "
    s = gets
    if s 
      puts "In dictionary: #{bf.include? s.chomp}"
    else
      done = true
    end
  end
  puts
end

