package java2c;

import java.util.Comparator;
import java.lang.reflect.Field;

public class FieldComparator implements Comparator<Field> {

    public int compare(Field field1, Field field2) {

        if (field1 == null && field2 == null) {
            return 0;
        }
        if (field1 == null) {
            return -1;
        }
        if (field2 == null) {
            return 1;
        }

        return field1.getName().compareTo(field2.getName());
   }
}
