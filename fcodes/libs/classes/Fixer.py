from fcodes.libs.classes.Fcode import FcodeManager
import fcodes.libs.data.keys as ky
import re

class Fixer:
    def __init__(self, fcode: str) -> None:
        self.code = fcode
        self.fcode = FcodeManager(fcode)
        self.wrong_patterns = self.load_wrong_patterns()
    
    def get_pattern_index(self, pattern: str)-> tuple:
        """Return the start and end layer index where the pattern occurs. Return
        a tuple with the format: (start_index, end_index) if the pattern is
        found, if not return an empty tuple (). Works only for f_lineages.
        """
        p_starts = self.fcode.f_linage.find(pattern)
        if p_starts == -1:
            return ()
        else:
            p_ends = p_starts + len(pattern) - 1
            return (p_starts, p_ends)

    def get_fcode_samples(self, **kwargs) -> list:
        """Parses self.fcode in samples of 1, 2, 3, ..., fcode.depth and returns
        a list with each sample. A dictionary with FcodeManager attributes can
        be passed as kwargs to get samples of that attribute. E.g.:
            fs = Fixer('*PA3o1o2')
            samples = fs.get_fcode_samples(**{'f_linage':''})
        """
        flen = self.fcode.depth
        sample_sizes = range(1, flen)
        samples = []
        for size in sample_sizes:
            for layer_num in range(flen):
                if size + layer_num <= flen:
                    if kwargs != {}:
                        kw_keys = [k for k in kwargs.keys()]
                        sel_sample = [self.fcode.__dict__[i] for i in kw_keys]
                        for s in sel_sample:
                            sample = s[layer_num:layer_num + size]
                            samples.append(sample)
                    else:
                        sample = self.fcode[layer_num:layer_num + size]
                        samples.append(sample)
                    
        return samples

    def load_wrong_patterns(self) -> list:
        """Return a list with the wrong_patterns found in the fcode. This
        function uses libs.data.keys.forbiden_lineage_patterns as reference
        for the patterns.
        """
        forbidden = ky.forbidden_lineage_patterns
        lineage_samples = self.get_fcode_samples(**{'f_linage':''})
        wrong_patterns = []
        for pattern in forbidden:
            if pattern in lineage_samples:
                wrong_patterns.append(pattern)
        return wrong_patterns

    def fix_up_numbers(self):
        """Removes numbers of P, M and C if they are not in the last layer"""

    def fix_pattern_XhX(self)-> str: #type: ignore
        """ Fix the structure of self.fcode with the pattern "hX",
            e.g.:   
                    *o1P1   --> *      (*hX   --> *   )
                    *PPa1P2 --> *PP2   (*XXhX --> *XX) 
                    *PPa1M2 --> *PPC   (*XXhX --> *XXC)
        """
        pat = self.get_pattern_index('XhX')
        pattern_start = self.fcode[pat[0]]
        pattern_ends = self.fcode[pat[1]]
        pattern_start_lin = self.fcode.f_linage[pat[0]]
        pattern_ends_lin = self.fcode.f_linage[pat[1]]
        pattern_start_bool = self.fcode.f_bool[pat[0]]
        pattern_ends_bool = self.fcode.f_bool[pat[1]]
        same_f_lineages = pattern_start_lin == pattern_ends_lin
        same_f_bool = pattern_start_bool == pattern_ends_bool
        # *XXhX --> *XX
        if pattern_ends_lin == ky.legend['parents']:
            # *PhP / *MhM --> *P / *M
            if same_f_bool:
                result = ''.join(self.fcode[0:pat[0]]
                                 + self.fcode[pat[1]:])
                return result
            # *PhM / *MhP --> *PC / *MC
            elif not same_f_bool \
                    and pattern_start != ky.legend['seed']:
                result = ''.join(self.fcode[0:pat[0]]
                                 + pattern_start
                                 + ky.legend['partner']
                                 + self.fcode[pat[1]+1:])
                return result