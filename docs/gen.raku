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

class Doc is Element {
    method gist {
        my $out = "Doc<\n";
        for @.children -> $child {
            $out ~= $child.gist.indent(1) ~ "\n"
        }
        $out ~ ">"
    }
}

class Section is Element {
    has $.title is rw;
    has $.level is rw;

    method gist {
        my $out = "Section<'$.title', $.level";
        for @.children -> $child {
            $out ~= $child.gist.indent(1) ~ "\n";
        }
        $out ~ ">"
    }
}

class Code is Element {
    has $.body is rw;
    has $.is_inline is rw;

    method gist {
        my $out = "Code<\n";
        $out ~= $.body.indent(1) ~ "\n";
        $out ~ ">"
    }
}

class Title is Element {
    has $.title is rw;

    method gist {
        "Title<$.title>"
    }
}

class Text is Element {
    has $.raw is rw;

    method gist {
        my $out = "Text<";
        if $.raw.chars > 30 {
            $out ~= $.raw.comb[0..30].join ~ "...";
        } else {
            $out ~= $.raw;
        }
        $out ~= ">";
        $out
    }
}

class Ref is Element {
    has $.target is rw;
    has $.body is rw;

    method gist {
        "Ref<'$.target', $.body>"
    }
}

class Anchor is Element {
    has @.names;
}

class Body is Element {
    method gist {
        my $out = "Body<\n";
        for @.children -> $child {
            $out ~= $child.gist.indent(1) ~ "\n";
        }
        $out ~ ">"
    }
}

class HTML is Element {
    has $.body is rw;
}

my @element_stack = [];

my @section-stack = [];

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
    body($doc);
}

sub body($parent) {
    my $body = Body.new;
    given $file {
        when /^\{\{/ {
            given eat(/\{\{\s*(\w+)/)[0] {
                when 'title' {title($body)}
                when 'anchor' {anchor($body)}
                when 'feature' {feature($body)}
                when 'html' {html($body)}
                default {
                    say $_;
                    die "$line:$col: command '$_' cannot be used inside a section body";
                }
            }
        }

        when /^\=/ {
            say "section";
            section($body);
        }

        when /^\`/ {
            say "code";
            code($body);
        }

        when /^\[/ {
            say "ref";
            ref($body);
        }

        default {
            say "text";
            text($body);
        }
    }

    $parent.children.push: $body;
}

sub title($parent) {
    my $title = Title.new;
    skip_whitespace;
    if eat(
        /
            \s*
            \|
            \s*
            (.*?)
            \}\}
        /
    ) -> $match {
        $title.title = $match[0].Str.trim;
        $parent.children.push: $_;
    } else {
        text($parent);
    }
}

sub text($parent) {
    my $text = Text.new;
    $text.raw = eat_until(
        /
            \[   # ref start
          | \n   # newlines are separate
          | \{\{ # command
          | \`   # code
          | $    # end of file 
        /,
    True);
    $parent.children.push: $text;
}

sub ref($parent) {
    if eat(
        /   \[
            $<tag>=[.*?] # eat tag
            
            [ # optional rename
                \|
                $<rename>=[.*?]
            ]?

            \]\] # end of ref
            
            [ # if there are trailing characters we take those too
                $<trailing>=[\w+]
            ]?  
        /
    ) -> $match {
        my $ref = Ref.new;
        $ref.target = $match<tag>.Str.trim;

        if $match<rename>:exists {
            $ref.body = $match<rename>;
        } elsif $match<trailing>:exists {
            $ref.body ~= $ref.target ~ $match<trailing>;
        }
        $parent.children.push: $ref;
    } else {
        # if the ref is malformed, we need to eat the [[ because otherwise text will
        # stop thinking we're at a ref again
        # this kinda sucks though cause it removes the [[ from the output, but if the link
        # doesn't show then I guess that's indication enough of an error
        eat(/^\[\[?/);
        text($parent);
    }

}

sub anchor($parent) {
    say "anchors not handled yet";
}

sub feature($parent) {
    say "feature command not implemented yet";
    eat_until(/\}\}/, True);
}

sub html($parent) {
    if eat(
        /
        (.*?)
        \}\}
        /
    ) -> $match {
        my $html = HTML.new;
        $html.body = $match[0].Str;
        $parent.children.push: $html;
    } else {
        text($parent);
    }
}

sub section($parent) {
    if eat(
        /
            $<equals>=[\=+] # eat equals
            \h*?
            $<title>=[.*?]
            [ # optional id
                \h*?
                \{
                    \h*?
                    $<id>=[.*?]
                    \h*?
                \}
            ]?
            \h*?
            \n
        /
    ) -> $match {
        my $section = Section.new;
        $section.title = $match<title>.Str.trim;
        $section.level = $match<equals>.chars;
        if $match<id>:exists {
            $section.tag = $match<id>.Str.trim;
        }
        $parent.children.push: $section;
    } else {
        text($parent);
    }
}

sub code($parent) {
    if eat(/^\`\`\`/) -> $match {
        my $code = Code.new;
        $code.is_inline = False;
        $code.body = eat_until(/\`\`\`/, True).Str.trim;
        $code.body ~~ s:g/^^(\h*)/{say $0.chars; '&nbsp' x $0.chars}/;
        $parent.children.push: $code;
    } elsif eat(/^\`/) -> $match {
        my $code = Code.new;
        $code.is_inline = True;
        $code.body = eat_until(/\`/, True).Str.trim;
        $code.body ~~ s:g/\h/&nbsp/;
        $parent.children.push: $code;
    } else {
        text($parent);
    }
}

say $doc;

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
            

            my @lines = $elem.body.split("\n");
            for @lines -> $line {
                $out ~= "<div class=\"code_line\">" ~ $line ~ "</div>\n";
            }
            $out ~= "</div>\n";
        }

    }
}

spurt "doc.html", $out;