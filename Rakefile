require 'fileutils'

BIN_DIR = 'bin'

TOOLS = {
  'nsf2fmsq' => {
    dir: 'nsf2fmsq',
    binary: 'nsf2fmsq'
  },
  'fmsq_player' => {
    dir: 'sdl2_simple_player',
    binary: 'fmsq_player'
  },
  'nsf_6502_player' => {
    dir: '6502emu_sdl2_player',
    binary: 'nsf_6502_player'
  }
}

desc "Build all tools"
task :build do
  FileUtils.mkdir_p(BIN_DIR)

  TOOLS.each do |name, info|
    puts "\n#{'=' * 60}"
    puts "Building #{name}..."
    puts '=' * 60

    build_dir = "#{info[:dir]}/build"
    FileUtils.mkdir_p(build_dir)

    Dir.chdir(build_dir) do
      sh "cmake .. 2>&1 | tail -3"
      sh "make -j$(nproc)"
    end

    src = "#{build_dir}/#{info[:binary]}"
    dst = "#{BIN_DIR}/#{info[:binary]}"
    cp src, dst, verbose: true
    puts "#{name} -> #{dst}"
  end

  puts "\n#{'=' * 60}"
  puts "All tools built. Binaries in #{BIN_DIR}/:"
  Dir.glob("#{BIN_DIR}/*").each { |f| puts "  #{f}" }
end

desc "Clean all build directories"
task :clean do
  TOOLS.each do |name, info|
    build_dir = "#{info[:dir]}/build"
    if Dir.exist?(build_dir)
      puts "Cleaning #{build_dir}"
      FileUtils.rm_rf(build_dir)
    end
  end
  FileUtils.rm_rf(BIN_DIR)
  puts "Clean done"
end

desc "Rebuild (clean + build)"
task :rebuild => [:clean, :build]

task :default => :build
