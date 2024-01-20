# index
# @ 
#
#
#
#
#


start_time = Time.monotonic 

require "colorize"

# platform check at compile time
{% if !(flag?(:linux) || flag?(:win32)) %}
{%	raise("building is currently only supported on linux and win32") %}
{% end %}

struct Time
	def my_format
		self.to_s "%a, %b %-d %Y %H:%M:%S %^Z"
	end
end

struct Time::Span
	macro unit(u, a)
		if {{ u }} != 0
			str << "#{{{u}}}{{a}} "
			n_units -= 1
			if n_units == 0
				next str
			end
		end
	end

	def pretty 
		String.build do |str|
			n_units = 2
			unit(hours, h)
			unit(minutes, m)
			unit(seconds, s)
			unit(milliseconds, ms)

			remaining_microseconds = (microseconds.microseconds - milliseconds.milliseconds).microseconds
			if remaining_microseconds != 0
				str << "#{remaining_microseconds}µs "
				n_units -= 1
				if n_units == 0
					next str
				end
			end
		end[..-2]
	end
end

class Build

# configuration properties
property \
        platform = "",
         verbose = false,
       buildmode = "debug",
       profiling = "off",
  build_analysis = false,
         use_pch = false,
     gen_compcmd = false,
    preprocessor = "unknown",
        compiler = "unknown",
          linker = "unknown",
		max_jobs = System.cpu_count.as(UInt32),
       build_dir = "build",
   hide_greeting = false

# other stuff
property \
	known_platforms = {"linux","win32"},
known_preprocessors = {"cpp"},
	known_compilers = {"clang++","cl"},
	  known_linkers = {"clang++","link"}

macro vprint(x)
	if verbose
		puts "#{"verbose".colorize.magenta}: #{{{ x }}}"
	end
end

def fatal(x)
	puts <<-END 
	#{caller[1]}:
	#{"fatal".colorize.red}: #{x}
	END
	p self
	exit 1
end

def warn(x)
	print "warning".colorize.yellow, ": ", x, "\n"
end

def clean_and_quit
	puts "rm -rfd #{build_dir}/*"
	puts `rm -rfd #{build_dir}/*`
	exit 0
end

# set default programs depending on the platform
def set_defaults_from_platform
{% if flag? :linux %}
	self.preprocessor = "cpp"
	self.compiler = "clang++"
	self.linker = "clang++"
	self.platform = "linux"
{% elsif flag? :win32 %}
	self.preprocessor = "cpp"
	self.compiler = "cl"
	self.linker = "link"
	self.platform = "win32"
{% else %}
{%   raise "'defaults_from_platform' needs to be implemented for this platform " %}
{% end %}
end

def process_argv
	vprint "processing ARGV"
	args = ARGV.each
	loop do
		case args.next
		when args.stop then break
		when "clean"  then clean_and_quit
		when "-v"     then self.verbose = true
		when "-r"     then self.buildmode = "release"
		when "-p"     then self.profiling = "on"
		when "-pw"    then self.profiling = "on and wait"
		when "-ba"    then self.build_analysis = true
		when "-pch"   then self.use_pch = true
		when "-cc"    then self.gen_compcmd = true
		when "-platform"
			plat = args.next
			if plat.is_a? Iterator::Stop
				fatal "expected a platform after '-platform'"
			end
			unless known_platforms.any? plat
				fatal <<-END
				unknown platform #{plat}. Known platformers are: #{known_platforms}
				New platforms can be implemented in tools/src/build.cr.
				END
			end
			self.platform = plat
		when "-preprocessor"
			p = args.next
			if p.is_a? Iterator::Stop
				fatal "expected a preprocessor after '-preprocessor'"
			end
			unless known_preprocessors.any? p
				fatal <<-END
				unknown preprocessor #{p}. Known preprocessors are: #{known_preprocessors}
				New preprocessors can be implemented in tools/src/build.cr.
				END
			end
			self.preprocessor = p
		when "-compiler"
			c = args.next
			if c.is_a? Iterator::Stop
				fatal "expected a compiler after '-compiler'"
			end
			unless known_compilers.any? c
				fatal <<-END 
				unknown compiler '#{c}'. Known compilers are: #{known_compilers}
				New compilers can be implemented in tools/src/build.cr.\n
				END
			end
			self.compiler = c
		when "-linker"
			l = args.next
			if l.is_a? Iterator::Stop
				fatal "expected a linker after '-linker'"
			end
			unless known_linkers.any? l
				fatal <<-END
				unknown linker '#{l}'. Known linkers are: #{known_linkers}
				New linkers can be implemented in tools/src/build.cr.\n
				END
			end
			linker = l
		when "-jobs"
			ns = args.next
			if ns.is_a? Iterator::Stop
				fatal "expected some number of jobs after '-jobs'"
			end
			if n = ns.to_u32?
				unless n < System.cpu_count
					warn "the given number of jobs (#{n}) is greater than the number of logical cores on this system (#{System.cpu_count})"
				end
				self.max_jobs = n
			else
				fatal "expected a number after '-jobs', not "
			end
		end
	end
end

# any function after this point should only be called after args have been collected

def output_path
	Path[build_dir] / buildmode
end

def output_exe
	output_path / "amu"
end

def create_output_path
	Dir.mkdir_p output_path
end

# display a nice message
def greeting
	puts <<-END
	* --- #{Time.local.my_format} --- *
	#{"amu".colorize.green} [ #{compiler.colorize.blue}/#{linker.colorize.blue}/#{buildmode.colorize.blue} ]
	#{
		if File.exists? output_exe
			modtime = File.info(output_exe).modification_time.to_local
			since = (Time.local - modtime)
			"last build: #{modtime.my_format} (#{since.pretty} ago)"
		end
	}
	END
end

@defines = [] of String

# --defines
def defines
	if @defines.empty?
		@defines = case buildmode
		when "release" then %w()
		when "debug"   then %w(-DAMU_DEBUG -DAMU_ENABLE_TRACE)
		else fatal "unhandled buildmode"
		end |
		case platform
		when "linux" then %w(-DAMU_LINUX=1)
		when "win32" then %w(-DAMU_WINDOWS=1)
		else fatal "unhandled platform"
		end | 
		case profiling
		when "on" then %w(-DAMU_PROFILING=1)
		when "on and wait" then %w(-DAMU_PROFILING=1 -DAMU_PROFILING_WAIT=1)
		when "off" then %w()
		else fatal "unhandled profiling mode"
		end
		vprint "built defines: #{@defines.join " "}"
	end
	@defines
end

@includes = [] of String

# --includes
def includes
	if @includes.empty?
		@includes = case compiler
		when "clang++", "cl"
			%w(
				-Isrc
			)
		else fatal "unhandled compiler #{compiler}"
		end
		vprint "built includes: #{@includes.join " "}"
	end
	@includes
end

# TODO(sushi) add libs generator when needed

# --pch
def pch
	case compiler
	when "clang++"
		fatal "TODO(sushi) automatic pch generation and recompilation, ideally incremental like obj files"
	else 
		fatal "precompiled headers are not implemented for compiler '#{compiler}"
	end
end

@compiler_flags = [] of String

# --compiler_flags
def compiler_flags
	if @compiler_flags.empty?
		@compiler_flags = case compiler
		when "clang++"
			%w(
			-std=c++20
			-finline-functions
			-pipe
			-msse3
			-fno-caret-diagnostics
			-fdiagnostics-color=always
			) | 
		   case buildmode
			when "release" then %w(-O2)
			when "debug" 
				%w(
				-fdebug-macro
				-ggdb3
				-O0
				)
			else fatal "unhandled buildmode"
			end
		when "cl"
			%w(
			-diagnostics:column
			-EHsc
			-nologo
			-MP
			-Oi
			-GR
			-std:c++20
			-utf-8
			) | 
		   case buildmode
			when "release" then %w(-O2)
			when "debug" 
				%w(
				-Z7
				-Od
				)
			else fatal "unhandled buildmode"
			end
		else fatal "compiler flags not setup for '#{compiler}'"
		end
		vprint "built compiler flags: #{@compiler_flags.join " "}"
	end
	@compiler_flags
end

# cache the dependencies to the file 'outfilename'
# the file is a newline separated list of headers 
# included by each cpp file
# --cache_dependencies
def cache_dependencies(filename, outfilename)
	vprint "caching dependencies for #{filename} to #{outfilename}"
	case preprocessor
	when "cpp"
		cmd = "cpp #{filename} #{@defines.join " "} #{@includes.join " "} -MM -std=c++20"
		vprint "preprocessor cmd: #{cmd}"
		dependencies = `#{cmd}`
			.split(" ", remove_empty: true)
			.skip(2)
			.each
			.map { |x| x.strip }
			.select { |x| x != "\\" }
			.join "\n"

		output = <<-END
			#{dependencies}\n
			END

		File.write outfilename, output

		return output
	else fatal "unhandled preprocessor #{preprocessor}"
	end
end

# --get_dependencies
def get_dependencies(filename, outfilename) : Array(String)
	lines = [] of String

	if File.exists? outfilename
		if File.info(filename).modification_time > File.info(outfilename).modification_time
			lines = cache_dependencies(filename, outfilename).lines
		else
			lines = File.read_lines(outfilename)
		end
	else
		lines = cache_dependencies(filename, outfilename).lines
	end

	return lines[1..]
end

@source_files = [] of Path
# object files we'll end up using in linking
# this is so high up because if a file is older than
# its object file we still need to add it to this list
@obj_files = [] of String

# appropriately retrieve the object file from the output path
# --get_object_file_path
def get_object_file_path(source_file : Path)
	case compiler
	when "clang++"
		return output_path / "#{source_file.stem}.o"
	when "cl"
		return output_path / "#{source_file.stem}.obj"
	else fatal "unhandled compiler #{compiler}"
	end
end

# find source files to pass to the compiler and filter out ones
# that are not necessary to compile
# --source_files
def source_files
	if @source_files.empty?
		files = Dir.glob("src/**/*.cpp") # find all cpp files
		@source_files = files.compact_map do |f| # filter out certain files
			f = Path[f].normalize
			# NOTE(sushi) temp until I fix up the code base
			if f.stem.includes? ".old"
				next nil
			end

			obj_file = get_object_file_path(f)
			mm_file = output_path / "#{f.stem}.mm"

			if File.exists? obj_file
				obj_modtime = File.info(obj_file).modification_time
				if File.info(f).modification_time > obj_modtime
					vprint "building '#{f}' because it is newer than its object file '#{obj_file}'"
					next f
				end

				deps = get_dependencies(f, mm_file)
				include_modified = deps.each.any? do |dep|
					if File.info(dep).modification_time > obj_modtime
						break true
					end
				end
				if include_modified
					# TODO(sushi) show the include 
					vprint "building '#{f}' because an include it depends on has been modified"
					next f
				end

				@obj_files.push obj_file.to_s
				next nil
			else
				cache_dependencies(f, mm_file)
				vprint "building '#{f}' because an object file does not exist yet"
				next f
			end
		end
	end
	@source_files
end

# depending on the selected compiler start
# a build analyzer. The way this is structured
# probably won't work properly for other compilers
# (as in, the fact that it is a separate process that
# is started and stopped), but it's how it works for clang
# and as far as I've seen vcperf also works like this,
# though I'm not totally sure
# --start_build_analyzer
def start_build_analyzer
	case compiler
	when "clang++"
		unless Process.find_executable "ClangBuildAnalyzer"
			fatal "build analysis is enabled (-ba) but ClangBuildAnalyzer could not be found on the system"
		end
		puts `ClangBuildAnalyzer --start #{output_path}`
		@compiler_flags.push "-ftime-trace"
	else fatal "unhandled compiler #{compiler}"
	end
end

# --stop_build_analyzer
def stop_build_analyzer
	output = output_path / "buildanalysis"
	case compiler 
	when "clang++"
		`ClangBuildAnalyzer --stop #{output_path} #{output}`
		puts `ClangBuildAnalyzer --analyze #{output}`
	else fatal "unhandled compiler #{compiler}"
	end
end

struct Command
	property \
	command = "",
	args = [] of String,
	from = "",
	to = ""
end	

@commands = [] of Command

# --generate_compiler_commands
def generate_compiler_commands
	case @compiler
	when "clang++"
		source_files.each do |sf|
			c = Command.new
			c.command = "clang++"
			c.from = sf.to_s
			c.to = get_object_file_path(sf).to_s
			c.args = ["-c", c.from, "-o", c.to, *@includes, *@defines, *@compiler_flags]
			@commands.push c
		end
	when "cl"
		source_files.each do |sf|
			c = Command.new
			c.command = "cl"
			c.from = sf.to_s
			c.to = get_object_file_path(sf).to_s
			c.args = ["-c", c.from, "-Fo" + c.to, *@includes, *@defines, *@compiler_flags]
			@commands.push c
		end
	else fatal "unhandled compiler '#{@compiler}'"
	end
end

def execute_compiler_commands
	# commands are send to fibers via this channel
	# the end is signaled by nil
	command_channel = Channel(Command | Nil).new(@commands.size)
	# channel which fibers use to notify the main fiber that they
	# are done
	done_signal = Channel(Nil).new
	
	# sentinel value returned to indicate if we should try
	# linking or not
	success = true

	# spawn as many fibers as we are allowed jobs
	max_jobs.times do
		spawn do
			# pipes for this fiber to get process output from
			stdout, stderr = IO::Memory.new, IO::Memory.new
			loop do
				c = command_channel.receive
				break if c.is_a? Nil
				vprint "#{c.command} #{c.args.join " "}"
				start_time = Time.monotonic
				# start the process and then wait for it to finish
				proc = Process.new(c.command, c.args, output: stdout, error: stderr)
				result = proc.wait
				elapsed_time = Time.monotonic - start_time
				if result.success?
					@obj_files.push c.to unless @obj_files.any? c.to
					print "#{c.from.colorize.cyan} -> #{c.to.colorize.green} #{elapsed_time.pretty}\n"
					# if we find a warning anywhere in the stderr, print it
					# TODO(sushi) setup doing this for various compilers
					if (t = stderr.to_s).includes? "warning"
						puts t
					end
				else
					success = false
					print <<-END
					#{c.from.colorize.yellow} #{"failed".colorize.red}:
					#{
						case @compiler
						when "clang++"
							stderr
						when "cl"
							stdout
						else fatal "unhandled compiler '@compiler'"
						end
					}
					END
				end
				# signal the main fiber that we've finished a job
				done_signal.send(nil)
			end
		end
	end

	# queue up each command
	@commands.each do |c|
		command_channel.send c
	end
	command_channel.send nil

	# we expect one done signal for each command 
	# so we wait on the done signal that many times
	@commands.size.times { done_signal.receive }

	return success
end

# currently linking only happens via one command
# --do_linking
def	do_linking
	stdout, stderr = IO::Memory.new, IO::Memory.new
	case linker
	when "clang++"
		output = output_path / "amu"
		args = [*@obj_files, "-o", output.to_s]
		vprint "clang++ #{args.join " "}"
		time_start = Time.monotonic
		proc = Process.new("clang++", args, output: stdout, error: stderr)
		result = proc.wait
		elapsed_time = Time.monotonic - time_start
		if result.success?
			puts "#{"amu".colorize.green} #{elapsed_time.pretty}"
		else 
			puts "#{"linking failed".colorize.red}:"
			# we wanna delete the object files that failed so that we dont 
			# try and reuse them in future build attempts
			stderr.to_s.each_line do |l|
				puts l
				m = l.match /(\w+\.o)/
				next unless m
				File.delete? output_path / m[1]
			end
		end
	when "link"
		output_exe = output_path / "amu.exe"
		output_pdb = output_path / "amu.pdb"
		args = [*@obj_files, "-DEBUG:FULL", "-OUT:" + output_exe.to_s, "-PDB:" + output_pdb.to_s]
		vprint "link #{args.join " "}"
		time_start = Time.monotonic
		proc = Process.new("link", args, output: stdout, error: stderr)
		result = proc.wait
		elapsed_time = Time.monotonic - time_start
		if result.success?
			puts "#{"amu".colorize.green} #{elapsed_time.pretty}"
		else 
			puts "#{"linking failed".colorize.red}:"
			# we wanna delete the object files that failed so that we dont 
			# try and reuse them in future build attempts
			stderr.to_s.each_line do |l|
				puts l
				m = l.match /(\w+\.obj)/
				next unless m
				File.delete? output_path / m[1]
			end
		end
	else fatal "unhandled linker '#{linker}'"
	end
end

def report_time(start_time)
	puts "build took: #{(Time.monotonic - start_time).pretty}"
end

def start
	start_time = Time.monotonic
	set_defaults_from_platform
	process_argv
	create_output_path
	greeting
	defines
	includes
	compiler_flags
	source_files
	if @source_files.empty? && File.exists? output_exe
		puts "Nothing to do."
		report_time(start_time)
		exit 0
	end
	start_build_analyzer if build_analysis
	generate_compiler_commands
	execute_compiler_commands &&
	do_linking
	stop_build_analyzer if build_analysis
	report_time(start_time)
end

end

Build.new.start
