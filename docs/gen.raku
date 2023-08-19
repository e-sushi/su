my $file = "docs.ad".IO.slurp;
$file ~~ s/\</&lt;/;
$file ~~ s/\>/&gt;/;

my $line = 1;
my $col = 1;
my $base_font_size = 11;

class Element {
    has %.style is rw;
    has $.tag is rw;
    has @.children is rw;
}

class Doc is Element {}

class Section is Element {
    has $.title is rw;
    has $.stopper is rw;
}

class Code is Element {
    has $.body is rw;
    has $.stopper is rw;
}

class Title is Element {
    has $.title is rw;
}

class Body is Element {}

class Text is Element {
    has $.raw is rw;
}

class Ref is Element {
    has $.target is rw;
    has $.body is rw;
}

my @element_stack = [];

my @section_number_stack = [1];
my @section-stop-stack = [];

# eats the file up to the match and returns the prematch
sub eat_until($re, $inclusive = False) {
    my $match = $file ~~ m/<$re>/;
    return False if not $match;
    my $pre = $match.prematch;
    my $lines = $pre.comb: "\n";
    if $lines {
        $line += $lines.elems;
        $col = $lines[*-1].chars + 1;
    } else {
        $col += $match.chars;
    }
    if $inclusive {
        $file .= substr($pre.chars + $match.chars, *);
    } else {
        $file .= substr($pre.chars, *);
    }
    return $pre;
}

# eats a match from the beginning of $file and returns it
sub eat($re) {
    my $match = $file ~~ m/^$<blah>=<$re>/;
    return False if not $match;
    my $lines = $match.comb: "\n";
    if $lines {
        $line += $lines.elems;
        $col = $lines[*-1].chars + 1;
    } else {
        $col += $match.chars;
    }
    $file .= substr($match.chars, *);
    return $match<blah>;
}

sub skip_whitespace {
    eat_until(/\S/);
}

my Int $section_nests = 0;

my $doc = Doc.new;

while $file {
    eat_until(/\\/, True);
    doc($doc);
    eat_until(/\S||$/)
}

sub doc($parent) {
    say "doc";
    skip_whitespace;
    given eat(/\w+/) {
        when 'title' {title($parent)}
        when 'section' {section($parent)}
        default {
            die "$line:$col: command $_ is not allowed in doc scope, $file";
        }
    }
}

sub body($parent) {
    my $body = Body.new;
    skip_whitespace;
    given $file.comb[0] {
        when '\\' {
            given eat(/\\(\w+)/)[0] {
                when 'code' {code($body)}
                when 'section' {section($body)}
                default {
                    die "$line:$col: command '$_' cannot be used inside a section body";
                }
            }
        }

        when '[' {
            ref($body);
        }

        default {
            text($body);
        }
    }

    $parent.children.push: $body;
}


sub title($parent) {
    say "title";
    $_ = Title.new;
    skip_whitespace;
    .title = eat(/(.*?)\n/)[0].Str.trim;
    $parent.children.push: $_;
}

sub text($parent) {
    $_ = Text.new;
    .raw = eat_until(/\[||\n||\\/);
    $parent.children.push: $_;
}

sub ref($parent) {
    $_ = Ref.new;
    if eat(/\[$<target>=[.*?] [\|$<body>=[.*?]]? \]/) -> $match {
        .target = $match<target>.Str.trim;
        .body = $match<body>.Str.trim;
        $parent.children.push: $_;
    } else {
        text($parent);
    }

}

sub section($parent) {
    $_ = Section.new;
    skip_whitespace;
    if eat(/\[(.*?)\]/) -> $match {
        .tag = $match[0].Str;
    }
    skip_whitespace;
    .title = eat(/(\w+)||\"(.*?)\"/)[0].Str.trim;
    skip_whitespace;
    say "sect ", .title;

    if $file.starts-with: '{' {
        .stopper = '}';
        $file .= substr(1, *);
        $col += 1;
    } else {
        .stopper = eat(/(.*?)\n/)[0];
    }

    $parent.children.push: $_;

    my $sect = $_;
    loop {
        body($sect);
        skip_whitespace;
        if $file.starts-with($sect.stopper) {
            $file .= substr($sect.stopper.chars, *);
            $col += $sect.stopper.chars;
            last;
        }
    }
    
    $section_nests += 1;
    say "sect ", $sect.title, " finished";

}

sub code($parent) {
    say "code";
    $_ = Code.new;
    skip_whitespace;
    .stopper = eat(/(.*?)\n/)[0].Str;
    .body = eat_until(/$($_.stopper)/, True).Str.trim;
    $parent.children.push: $_;
}

print_elems($doc);
sub print_elems($elem) {
    say $elem, "\n";
    for $elem.children -> $child {
        print_elems($child);
    }
}

my $out = q:to/END/;
<style>

    .title {
        font-size: 30px;
        text-align: center;
        padding: 10px;
    }

    .section_header {
        left: 10px;
        font-size: 20px;
        margin: 5px;
    }

    .section_body {
        padding-left: 15px;
    }
    
    .code {
        padding: 10px;
        background-color: 
    }

</style>
END

output($doc);
sub output($elem) {

    given $elem {
        when Doc {
            output($_) for $elem.children;
        }

        when Title {
            $out ~= "<div class=\"title\">{$elem.title}</div>\n"; 
        } 

        when Section {
            $out ~= "\n<div class=\"section_header\"" ~
            do if $elem.tag { " id=\"{$elem.tag}\"" } ~
            ">{$elem.title}</div>\n";

            $out ~= "<div class=\"section_body\">\n";
            
            for $elem.children -> $child {
                output($child);
            }

            $out ~= "\n</div>\n";
        }

        when Body {
            output($_) for $elem.children;
        }

        when Text {
            $out ~= "{$elem.raw}";
        }

        when Ref {
            $out ~= "<a href=\"#{$elem.target}\">";
            if $elem.body {
                $out ~= $elem.body;
            } else {
                $out ~= $elem.target;
            }
            $out ~= "</a>";
        }

        when Code {
            $out ~= "\n<div class=\"code\">\n";
            say $elem.body.raku;
            my @lines = $elem.body.split("\n");
            for @lines -> $line {
                $out ~= "<div class=\"code_line\">" ~ $line ~ "</div>\n";
            }
            $out ~= "</div>\n";
        }

    }
}

spurt "doc.html", $out;