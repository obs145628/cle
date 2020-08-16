export class Rect {


    constructor(left, top, width, height) {
        this.x0 = left;
        this.y0 = top;
        this.w = width;
        this.h = height;
    }

    static from_ltwh(obj) {
        return new Rect(obj.left, obj.top, obj.width, obj.height);
    }

    static merge(a, b) {
        // merge 2 recs that are side by side in one
        if (a.x0 === b.x0) //y merge
            return new Rect(a.x0, Math.min(a.y0, b.y0), a.w, 2 * a.h); 
        
        else //x merge
            return new Rect(Math.min(a.x0, b.x0), a.y0, 2 * a.w, a.h);
    }

    // Divide rect into 2 rects with same height, width / 2
    split_x() {
        return [
            new Rect(this.x0, this.y0, this.w / 2, this.h),
            new Rect(this.x0 + this.w / 2, this.y0, this.w / 2, this.h),
        ];
    }

    // Divide rect into 2 rects with same width, height / 2
    split_y() {
        return [
            new Rect(this.x0, this.y0, this.w, this.h / 2),
            new Rect(this.x0, this.y0 + this.h / 2, this.w, this.h / 2),
        ];
    }

    to_lthw() {
        return {
            left: this.x0,
            top: this.y0,
            width: this.w,
            height: this.h,
        };
    }

}