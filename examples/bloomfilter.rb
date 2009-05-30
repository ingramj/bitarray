require 'digest/sha1'
require 'digest/md5'
require 'bitarray'

# This bloom filter was written as a demonstration of the BitArray class.
# Therefore, it was written with an eye towards simplicity rather than
# efficiency or optimization.
#
# The size of the filter is configurable, but we always use 10 hashes.
# 
# For information about picking parameters for a Bloom Filter, take a look at
# the Wikipedia page.
#
# http://en.wikipedia.org/wiki/Bloom_filter
#
class BloomFilter
  def initialize(m = 1000000)
    @size = m
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
  # Return an array of 10 indices to set.
  def hash(input)
    # We use the SHA1 hash of the input, and divide into five chunks.
    h = Digest::SHA1.hexdigest(input)
    ha = [ h[0,8], h[8,8], h[16,8], h[24,8], h[32,8] ]
    # Do the same with the MD5 hash, divided into four chunks.
    h = Digest::MD5.hexdigest(input)
    ha += [ h[0,8], h[8,8], h[16,8], h[24,8] ]
    # And add in the built-in (murmur) hash.
    ha.map! {|hi| hi.to_i(16) % @size}
    ha << input.hash.abs % @size
    return ha
  end
end


# As a demonstration, load the contents of the dictionary file and let the user
# look up words. 
#
# Using the calculator at http://hur.st/bloomfilter, the optimum number of bits
# for my system's dictionary file (98569 words), with 10 hashes and a false
# positive rate of 0.001, is 1,417,185 bits (about 173 Kb).
#
# Loading the dictionary takes about 7 seconds on my system.
if __FILE__ == $0
  print "Loading dictionary..."
  bf = BloomFilter.new(1417185)
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

