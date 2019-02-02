#include "qcubicsegment1d.h"
#include "simplemath.h"
#include "qcubicsegment2d.h"

qreal qCubicSegment1D::valAtT(const qreal &t) const {
    return qPow(1 - t, 3)*p0() +
            3*qPow(1 - t, 2)*t*c1() +
            3*(1 - t)*t*t*c2() +
            t*t*t*p1();
}

qreal qCubicSegment1D::length() {
    if(!mLengthUpToDate) updateLength();
    return fLength;
}

qreal qCubicSegment1D::tAtLength(const qreal &len) {
    if(isZero6Dec(len) || len < 0) return 0;
    qreal totLen = length();
    if(isZero6Dec(len - totLen) || len > totLen) return 1;
    return tAtLength(len, 0.01, 0, 1);
}

qreal qCubicSegment1D::lengthAtT(qreal t) {
    t = CLAMP(t, 0, 1);
    if(isZero6Dec(qMax(0., t))) return 0;
    if(isZero6Dec(qMin(1., t) - 1)) return length();
    auto divSeg = dividedAtT(t);
    return divSeg.first.length();
}

qreal qCubicSegment1D::lengthFracAtT(qreal t) {
    t = CLAMP(t, 0, 1);
    if(isZero6Dec(t)) return 0;
    if(isZero6Dec(t - 1)) return length();
    qreal totLen = length();
    if(isZero6Dec(totLen)) return 1;
    return lengthAtT(t)/totLen;
}

qCubicSegment1D::Pair qCubicSegment1D::dividedAtT(qreal t) {
    t = CLAMP(t, 0, 1);
    qreal oneMinusT = 1 - t;
    qreal P0_1 = p0()*oneMinusT + c1()*t;
    qreal P1_2 = c1()*oneMinusT + c2()*t;
    qreal P2_3 = c2()*oneMinusT + p1()*t;

    qreal P01_12 = P0_1*oneMinusT + P1_2*t;
    qreal P12_23 = P1_2*oneMinusT + P2_3*t;

    qreal P0112_1223 = P01_12*oneMinusT + P12_23*t;

    qCubicSegment1D seg1(p0(), P0_1, P01_12, P0112_1223);
    qCubicSegment1D seg2(P0112_1223, P12_23, P2_3, p1());

    return {seg1, seg2};
}
#include "exceptions.h"

qCubicSegment1D qCubicSegment1D::tFragment(qreal minT, qreal maxT) {
    maxT = CLAMP01(maxT);
    minT = CLAMP01(minT);
    if(isZero6Dec(minT - maxT)) return qCubicSegment1D(valAtT(minT));
    if(minT > maxT) {
        RuntimeThrow("Wrong t range. Min value larger than max.");
    }
    if(isZero6Dec(minT - 1)) return qCubicSegment1D(mP1);
    qCubicSegment1D div1 = dividedAtT(minT).second;
    if(isZero6Dec(maxT - 1)) return div1;
    qreal mappedMaxT = (maxT - minT)/(1 - minT);
    return div1.dividedAtT(mappedMaxT).first;
}

const qreal &qCubicSegment1D::p0() const { return mP0; }

const qreal &qCubicSegment1D::c1() const { return mC1; }

const qreal &qCubicSegment1D::c2() const { return mC2; }

const qreal &qCubicSegment1D::p1() const { return mP1; }

void qCubicSegment1D::setP0(const qreal &p0) { mP0 = p0; }

void qCubicSegment1D::setC1(const qreal &c1) { mC1 = c1; }

void qCubicSegment1D::setC2(const qreal &c2) { mC2 = c2; }

void qCubicSegment1D::setP1(const qreal &p1) { mP1 = p1; }

qreal qCubicSegment1D::minDistanceTo(const qreal &p,
                                     qreal * const pBestT,
                                     qreal * const pBestPos) {
    return minDistanceTo(p, 0, 1, pBestT, pBestPos);
}

qreal qCubicSegment1D::minDistanceTo(const qreal &p,
                                     const qreal &minT,
                                     const qreal &maxT,
                                     qreal * const pBestT,
                                     qreal * const pBestPos) {
    qreal maxLen = lengthAtT(maxT);
    qreal bestT = 0;
    qreal bestPt = p0();
    qreal minError = DBL_MAX;
    for(qreal len = lengthAtT(minT); len < maxLen;) { // t ∈ [0., 1.]
        qreal t = tAtLength(len);
        qreal pt = valAtT(t);
        qreal dist = abs(pt - p);
        if(dist < minError) {
            bestT = t;
            bestPt = pt;
            minError = dist;
            if(minError < 1) {
                while(dist < minError) {
                    bestT = t;
                    bestPt = pt;
                    minError = dist;
                    qreal tMinusOne = t - 1;
                    qreal pow2TMinusOne = pow2(tMinusOne);
                    qreal pow3TMinusOne = pow3(tMinusOne);

                    qreal v0 = 3*c2() - 3*c2()*t + p1()*t;
                    qreal v2 = 3*c1()*pow2TMinusOne + t*v0;
                    qreal v4 = -1 + 4*t - 3*pow2(t);

                    qreal num = pow2(p + p0()*pow3TMinusOne - t*v2);
                    qreal den = -6*(p0()*pow2TMinusOne + t*(-2*c2() + 3*c2()*t - p1()*t) +
                                    c1()*v4)*
                                  (-p - p0()*pow3TMinusOne +  t*v2);
                    if(isZero6Dec(den)) {
//                        if(first && minError > 0.01 && !isZeroOrOne6Dec(t)) {
//                            t = gCubicTimeAtLength(seg, len + dist*0.5);
//                            pt = gCalcCubicBezierVal(seg, t);
//                            dist = pointToLen(pt - p);
//                            continue;
//                        }
                        break;
                    }
                    qreal newT = t - num/den;
                    pt = valAtT(newT);
                    dist = abs(pt - p);
                    t = newT;
                }
            }
            if(minError < 0.01) break;
        }

        len += dist*0.8;
    }
    if(pBestPos) *pBestPos = bestPt;
    if(pBestT) *pBestT = bestT;
    return minError;
}

qreal qCubicSegment1D::maxValue() const {
    return valAtT(tWithBiggestValue());
}

qreal qCubicSegment1D::minValue() const {
    return valAtT(tWithSmallestValue());
}

void qCubicSegment1D::solveDerivativeZero(
        qreal &t1, qreal &t2, qreal &t3) const {
    const qreal den = 3*c1() - 3*c2() + p1() - p0();
    const qreal num0 = 2*c1() - c2() - p0();
    const qreal numSqrt = sqrt(pow2(c1()) - p0()*c2() - c1()*c2() +
                         pow2(c2()) + p0()*p1() - c1()*p1());
    t1 = CLAMP01((num0 + numSqrt)/den);
    t2 = CLAMP01((num0 - numSqrt)/den);
    t3 = CLAMP01((3*c2() - 2*p0() - p1())/(6*c2() - 2*p0() - 4*p1()));
}

qreal qCubicSegment1D::tWithBiggestValue() const {
    const bool p0Further = p0() >= c1() && p0() >= c2();
    const bool p1Further = p1() >= c1() && p1() >= c2();
    if(p0Further || p1Further) {
        if(p1() > p0()) return 1;
        return 0;
    }
    qreal t1, t2, t3;
    solveDerivativeZero(t1, t2, t3);

    qreal maxVal = DBL_MIN;
    qreal bestT = 0;
    for(const qreal& t : { 0., 1., t1, t2, t3}) {
        const qreal valT = valAtT(t);
        if(valT > maxVal) {
            maxVal = valT;
            bestT = t;
        }
    }
    return bestT;
}

qreal qCubicSegment1D::tWithSmallestValue() const {
    const bool p0Further = p0() <= c1() && p0() <= c2();
    const bool p1Further = p1() <= c1() && p1() <= c2();
    if(p0Further || p1Further) {
        if(p1() < p0()) return 1;
        else return 0;
    }
    qreal t1, t2, t3;
    solveDerivativeZero(t1, t2, t3);

    qreal minVal = DBL_MAX;
    qreal bestT = 0;
    for(const qreal& t : { 0., 1., t1, t2, t3}) {
        const qreal valT = valAtT(t);
        if(valT < minVal) {
            minVal = valT;
            bestT = t;
        }
    }
    return bestT;
}

//QList<qrealPair> _qCubicSegment1D::sIntersectionTs(
//        _qCubicSegment1D &seg1, _qCubicSegment1D &seg2) {
//    QList<qrealPair> sols;
//    qreal totalLen1 = seg1.getLength();
//    qreal totalLen2 = seg2.getLength();

//    for(qreal len1 = 0; len1 < totalLen1;) { // t ∈ (0., 1.)
//        qreal t1 = seg1.tAtLength(len1);
//        QPointF pt1 = seg1.posAtT(t1);

//        qreal smallestDist = DBL_MAX;
//        for(qreal len2 = 0; len2 < totalLen2;) { // t ∈ (0., 1.)
//            qreal t2 = seg2.tAtLength(len2);
//            QPointF pt2 = seg2.posAtT(t2);
//            qreal dist = pointToLen(pt1 - pt2);
//            if(dist < smallestDist) smallestDist = dist;

//            if(dist < .5) {
//                sols.append({t1, t2});
//                len2 += 1.;
//                smallestDist += 1;
//                break;
//            }

//            len2 += dist*0.5;
//        }

//        len1 += smallestDist*0.5;
//    }

//    return sols;
//}

qreal qCubicSegment1D::tAtLength(const qreal &length,
                                 const qreal &maxLenErr,
                                 const qreal &minT,
                                 const qreal &maxT) {
    const qreal guessT = (maxT + minT)*0.5;
    const qreal lenAtGuess = lengthAtT(guessT);
    if(abs(lenAtGuess - length) < maxLenErr) return guessT;
    if(lenAtGuess > length) {
        return tAtLength(length, maxLenErr, minT, guessT);
    } else {
        return tAtLength(length, maxLenErr, guessT, maxT);
    }
}

void qCubicSegment1D::updateLength() {
    mLengthUpToDate = true;
    QPainterPath path;
    path.moveTo(p0(), 0);
    path.cubicTo(c1(), 0, c2(), 0, p1(), 0);
    fLength = path.length();
}