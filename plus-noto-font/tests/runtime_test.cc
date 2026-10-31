// SPDX-License-Identifier: MIT
#include "plus_font.h"
#include <algorithm>
#include <cassert>
#include <climits>
#include <fstream>
#include <iostream>
#include <iterator>
#include <string>
#include <vector>

using namespace plus_font;
namespace {
void Put16(std::vector<uint8_t>& b, std::size_t at, int n) {
    b[at] = uint8_t(n & 255); b[at+1] = uint8_t((n >> 8) & 255);
}
void Put32(std::vector<uint8_t>& b, std::size_t at, uint32_t n) {
    for (unsigned i=0; i<4; ++i) b[at+i] = uint8_t(n >> (i*8));
}
std::vector<uint8_t> Synthetic() {
    // Tiny independent fixture: negative bearing, odd 3-pixel width and a
    // whitespace glyph with no bitmap. Deliberately not a full Chinese pack.
    std::vector<uint8_t> b(128,0);
    b[0]='N'; b[1]='T'; b[2]='F'; b[3]='1';
    Put16(b,4,1); Put16(b,6,40); Put32(b,8,1); Put32(b,12,3);
    Put32(b,20,40); Put32(b,24,64); Put32(b,28,124); Put32(b,32,124); Put32(b,36,128);
    Put16(b,40,32); Put16(b,42,500); Put16(b,44,112);
    Put16(b,48,4); Put16(b,50,1); Put16(b,52,6); Put32(b,60,3);
    Put32(b,64,' '); Put32(b,68,124); Put16(b,80,4);
    Put32(b,84,'?'); Put32(b,88,124); Put16(b,92,3); Put16(b,94,3);
    Put16(b,98,-3); Put16(b,100,4);
    Put32(b,104,'A'); Put32(b,108,126); Put16(b,112,3); Put16(b,114,3);
    Put16(b,116,-1); Put16(b,118,-3); Put16(b,120,4);
    b[124]=0xa5; b[125]=0;  // 101 / 001 / 010
    b[126]=0xbe; b[127]=0x80; // 101 / 111 / 101
    return b;
}
bool Pixel(const Surface1bpp& s,int x,int y) {
    const bool bit = (s.data[std::size_t(y)*s.stride_bytes+std::size_t(x)/8] & (0x80>>(x&7))) != 0;
    return bit == s.one_is_black;
}
void CheckSynthetic() {
    const auto b=Synthetic(); Font f; assert(f.Open(b.data(),b.size()));
    assert(f.glyph_count()==3 && f.spec().threshold==112);
    assert(f.Contains('A') && !f.Contains(0x4e2d));
    // Padding bytes plus both buffer-end guards must stay untouched.
    std::vector<uint8_t> out(32+6*4+32,0x55);
    std::fill(out.begin()+32,out.begin()+32+24,0);
    Surface1bpp s={out.data()+32,24,13,6,4,true};
    auto r=DrawText(f,s,"A",1,2,4);
    assert(r.status==Status::Ok && r.width==4 && r.end_x==6 && r.height==5);
    const int matrix[3][3]={{1,0,1},{1,1,1},{1,0,1}};
    for(int y=0;y<6;++y)for(int x=0;x<13;++x)
        assert(Pixel(s,x,y)==(y>=1&&y<4&&x>=1&&x<4&&matrix[y-1][x-1]));
    for(int y=0;y<6;++y){assert(s.data[y*4+2]==0&&s.data[y*4+3]==0);assert((s.data[y*4+1]&7)==0);}
    assert(std::all_of(out.begin(),out.begin()+32,[](uint8_t n){return n==0x55;}));
    assert(std::all_of(out.end()-32,out.end(),[](uint8_t n){return n==0x55;}));
    // White ink clears only the same glyph's ink positions.
    std::fill(out.begin()+32,out.begin()+56,0xff);
    r=DrawText(f,s,"A",1,2,4,false); assert(r.status==Status::Ok);
    for(int y=0;y<6;++y)for(int x=0;x<13;++x)
        assert(Pixel(s,x,y)==!(y>=1&&y<4&&x>=1&&x<4&&matrix[y-1][x-1]));
    // Inverse surface polarity and clipping at top-left.
    s.one_is_black=false; std::fill(out.begin()+32,out.begin()+56,0xff);
    r=DrawText(f,s,"A",1,0,1); assert(r.status==Status::Ok);
    assert(!Pixel(s,0,0)&&Pixel(s,1,0));
    for(int y=0;y<6;++y)for(int x=0;x<13;++x)
        if(!(x==1&&y==0))assert(!Pixel(s,x,y));
    const std::string lines="AA\r\nA\tA\n";
    r=MeasureText(f,lines.data(),lines.size(),7,20);
    assert(r.status==Status::Ok&&r.lines==3&&r.width==24&&r.height==17&&r.end_x==7&&r.end_baseline==32);
    const std::string missing="\xe4\xb8\xad";
    r=MeasureText(f,missing.data(),missing.size());
    assert(r.missing_glyphs==1&&r.invalid_utf8==0&&r.width==4);
    const char invalid[]={'\xc0','\xaf','\xed','\xa0','\x80','\xf4','\x90','\x80','\x80','\xe4'};
    r=MeasureText(f,invalid,sizeof(invalid));
    assert(r.invalid_utf8==sizeof(invalid)&&r.missing_glyphs==0&&r.bytes_consumed==sizeof(invalid));
    assert(MeasureText(f,nullptr,0).status==Status::Ok);
    assert(MeasureText(f,nullptr,1).status==Status::InvalidText);
    assert(MeasureText(f,"",0).lines==0);
    assert(MeasureText(f,"A",1,INT32_MAX-2,0).status==Status::CoordinateOverflow);
    assert(MeasureText(f,"\n",1,0,INT32_MAX-2).status==Status::CoordinateOverflow);
    assert(DrawText(f,s,"A",1,INT32_MIN,INT32_MIN).status==Status::Ok);
    auto bads=s; bads.capacity_bytes=23; assert(DrawText(f,bads,"A",1,0,0).status==Status::InvalidSurface);
    bads=s; bads.stride_bytes=1; assert(DrawText(f,bads,"A",1,0,0).status==Status::InvalidSurface);
    bads=s; bads.width=-1; assert(DrawText(f,bads,"A",1,0,0).status==Status::InvalidSurface);
    // Every truncation of the independent fixture must fail cleanly.
    for(std::size_t n=0;n<b.size();++n){Font invalid_font;assert(!invalid_font.Open(b.data(),n));}
    // Exact header/spec/index/padding corruption; failing re-open clears state.
    for(std::size_t at:{std::size_t(0),std::size_t(4),std::size_t(6),std::size_t(8),std::size_t(16),
                       std::size_t(20),std::size_t(24),std::size_t(28),std::size_t(32),std::size_t(36),
                       std::size_t(46),std::size_t(54),std::size_t(56),std::size_t(60),std::size_t(68),
                       std::size_t(82),std::size_t(125),std::size_t(127)}) {
        auto bad=b; bad[at]^=1;
        assert(!f.Open(bad.data(),bad.size())); assert(!f.valid());
        assert(MeasureText(f,"A",1).status==Status::InvalidFont);
    }
    auto bad=b;Put32(bad,104,0xd800);assert(!f.Open(bad.data(),bad.size()));
    bad=b;Put32(bad,104,'?');assert(!f.Open(bad.data(),bad.size()));
    bad=b;Put16(bad,120,-1);assert(!f.Open(bad.data(),bad.size()));
    bad=b;Put32(bad,12,UINT32_MAX);assert(!f.Open(bad.data(),bad.size()));
    bad=b;Put16(bad,52,1);assert(!f.Open(bad.data(),bad.size()));
    // Glyphs may end above the baseline (e.g. quotation marks), not just at it.
    bad=b;Put16(bad,98,-4);assert(f.Open(bad.data(),bad.size()));
}

void CheckFile(const char* path) {
    std::ifstream file(path,std::ios::binary); assert(file.good());
    std::vector<uint8_t> b((std::istreambuf_iterator<char>(file)),{});
    Font f; assert(f.Open(b.data(),b.size()));
    assert(f.Contains('?')&&f.Contains(' ')&&f.Contains('A'));
    const char sample[]="中文 Aa09，。\nTest 123";
    std::vector<uint8_t> guard(32+154*100+32,0xa5);
    std::fill(guard.begin()+32,guard.end()-32,0);
    Surface1bpp s={guard.data()+32,154*100,1216,100,154,true};
    auto measured=MeasureText(f,sample,sizeof(sample)-1,10,60);
    auto drawn=DrawText(f,s,sample,sizeof(sample)-1,10,60);
    assert(measured.status==Status::Ok&&drawn.status==Status::Ok);
    assert(measured.width==drawn.width&&measured.height==drawn.height&&measured.end_x==drawn.end_x);
    assert(measured.bytes_consumed==drawn.bytes_consumed&&measured.invalid_utf8==0);
    assert(std::all_of(guard.begin(),guard.begin()+32,[](uint8_t n){return n==0xa5;}));
    assert(std::all_of(guard.end()-32,guard.end(),[](uint8_t n){return n==0xa5;}));
    // Compare every pixel of a glyph rendered via the production API to the
    // glyph accessor. This exercises non-row-aligned bits on real font data.
    Glyph g={};assert(f.Find('A',&g));
    std::vector<uint8_t> tile(100*20,0);Surface1bpp t={tile.data(),tile.size(),137,100,20,true};
    const int x=20,base=70;
    assert(DrawText(f,t,"A",1,x,base).status==Status::Ok);
    for(int yy=0;yy<100;++yy)for(int xx=0;xx<137;++xx){
        const int gx=xx-x-g.x_offset,gy=yy-base-g.y_offset;
        bool expected=false;
        if(gx>=0&&gy>=0&&gx<g.width&&gy<g.height){const int bit=gy*g.width+gx;expected=(g.bitmap[bit/8]&(0x80>>(bit&7)))!=0;}
        assert(Pixel(t,xx,yy)==expected);
    }
    std::cout<<"PASS "<<path<<" "<<f.spec().pixels<<"px weight="<<f.spec().weight
             <<" threshold="<<f.spec().threshold<<" glyphs="<<f.glyph_count()<<"\n";
}
}
int main(int argc,char** argv) {
    CheckSynthetic();
    for(int i=1;i<argc;++i)CheckFile(argv[i]);
    std::cout<<"PASS runtime fixtures, UTF-8, original bearings, clipping, polarity, malformed packs and coordinate bounds\n";
}
