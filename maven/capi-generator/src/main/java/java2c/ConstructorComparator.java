package java2c;

import java.util.Comparator;
import java.lang.reflect.Constructor;

public class ConstructorComparator implements Comparator<Constructor> {

    private final CAPIGenerator capiGenerator;

    ConstructorComparator(CAPIGenerator capiGenerator) {
        this.capiGenerator = capiGenerator;
    }

    public int compare(Constructor constructor1, Constructor constructor2) {
        if (constructor1 == null && constructor2 == null) {
            return 0;
        }
        if (constructor1 == null) {
            return -1;
        }
        if (constructor2 == null) {
            return 1;
        }

        String sig1 = capiGenerator.getMethodName("new", constructor1.getParameterTypes());
        String sig2 = capiGenerator.getMethodName("new", constructor2.getParameterTypes());

        return sig1.compareTo(sig2);
    }
}
