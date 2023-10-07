my $obj = run(<clang -c get_context.s -o context.o>, :out).out.slurp: :close;
my $od = run(<objdump -M intel -d context.o>, :out).out.slurp: :close;

my @listings = $od ~~ m:g/^^<.xdigit>+\s*\<$<name>=[\w+]\>\:$<code>=[.*?]\n[\n||$]/;
for @listings -> $listing {
	my @lines = $listing<code> ~~ m:g/<?after \:\s*>$<code>=[[(<.xdigit>**2)\h]+]<?after \h*?>$<asm>=[.*?][\n||$]/;
	my $max = 0;
	my @linesf = [];
	for @lines -> $line {
		my $bytesf = [~] ("0x" <<~<< $line<code>.trim.split(" ") >>~>> ",");
		$max max= $bytesf.chars;
		@linesf.push: ($bytesf, $line<asm>.trim);
	}
	
	my $arr = qq:to/END/;
		__attribute__((section(".text")))
		static u8 {$listing<name>}_code_x86_64_sysv[] = \{
		END
	for @linesf -> $linef {
		$arr ~= "\t{$linef[0]}" ~ (" " x $max - $linef[0].chars) ~ " // {$linef[1]}\n";
	}
	$arr ~= "};";
	$arr.say;
}
